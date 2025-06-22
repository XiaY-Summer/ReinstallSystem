import 'dart:io';
import 'dart:async';
import '../providers/installer_provider.dart';

class InstallerService {
  final InstallerProvider provider;
  double _lastProgress = 0.0;
  bool _isInstallingPhase = false;
  int _installingStepCount = 0;
  
  // 调整权重以更好地反映实际过程
  static const double _preparationWeight = 0.1;  // 10% 用于初始准备
  static const double _peDownloadWeight = 0.2;   // 20% 用于PE下载
  static const double _isoDownloadWeight = 0.3;  // 30% 用于ISO下载
  static const double _installWeight = 0.4;      // 40% 用于实际安装
  
  // 安装阶段的步骤总数（根据WinInstaller.cpp中的ExecuteCommand调用次数）
  static const int _totalInstallingSteps = 7; // Rename.cmd, CreatPE.cmd, 7z, xcopy install.wim, xcopy script.cmd, xcopy DelPE.cmd, boot.cmd
  
  InstallerService(this.provider);

  Future<void> runInstaller() async {
    final args = _buildArguments();
    
    try {
      // 设置初始准备阶段的进度
      provider.setStatus(InstallStatus.preparing);
      _updateProgress(_preparationWeight);
      provider.setCurrentStep('准备安装环境...');

      final process = await Process.start(
        'WinInstaller.exe',
        args,
        runInShell: true,
      );

      // 处理标准输出
      process.stdout.transform(const SystemEncoding().decoder).listen((data) {
        _handleOutput(data);
      });

      // 处理标准错误
      process.stderr.transform(const SystemEncoding().decoder).listen((data) {
        provider.setErrorMessage(data);
      });

      // 等待进程完成
      final exitCode = await process.exitCode;
      if (exitCode != 0) {
        provider.setStatus(InstallStatus.error);
        provider.setErrorMessage('安装过程失败，退出代码：$exitCode');
      } else {
        provider.setStatus(InstallStatus.completed);
        provider.setProgress(1.0);
        provider.setCurrentStep('安装完成，即将重启...');
      }
    } catch (e) {
      provider.setStatus(InstallStatus.error);
      provider.setErrorMessage('启动安装程序失败：$e');
    }
  }

  List<String> _buildArguments() {
    final args = <String>[];

    // 系统选择
    args.add('--select');
    switch (provider.selectedSystem) {
      case SystemType.win10:
        args.add('win10');
        break;
      case SystemType.win11:
        args.add('win11');
        break;
      case SystemType.custom:
        args.add('custom');
        // 自定义镜像路径
        args.add('--path');
        args.add(provider.customImagePath);
        break;
    }

    // 镜像索引
    args.add('--set');
    args.add(provider.imageIndex.toString());

    // 驱动备份
    args.add('--backupdrive');
    args.add(provider.backupDrivers.toString());

    return args;
  }

  void _handleOutput(String data) {
    // 解析输出并更新状态
    if (data.contains('[EXEC]')) {
      final step = data.replaceAll('[EXEC]', '').trim();
      provider.setCurrentStep(step);
      
      // 检查是否进入安装阶段
      if (step.contains('tools\\')) {
        if (!_isInstallingPhase) {
          _isInstallingPhase = true;
          provider.setStatus(InstallStatus.installing);
          _installingStepCount = 0;
        }
        _installingStepCount++;
        
        // 计算安装阶段的进度
        final baseProgress = _preparationWeight + _peDownloadWeight + _isoDownloadWeight;
        final stepProgress = _installWeight * (_installingStepCount / _totalInstallingSteps);
        _updateProgress(baseProgress + stepProgress);
      }
    }
    
    // 根据不同阶段更新进度
    if (data.contains('下载开始')) {
      if (data.toLowerCase().contains('boot.wim')) {
        // PE下载开始
        provider.setStatus(InstallStatus.downloading);
        _updateProgress(_preparationWeight + (_peDownloadWeight * 0.1));
      } else if (data.toLowerCase().contains('.iso')) {
        // ISO下载开始
        _updateProgress(_preparationWeight + _peDownloadWeight + (_isoDownloadWeight * 0.1));
      }
    } else if (data.contains('MD5验证通过')) {
      if (data.toLowerCase().contains('boot.wim')) {
        // PE下载完成
        _updateProgress(_preparationWeight + _peDownloadWeight);
      } else if (data.toLowerCase().contains('.iso')) {
        // ISO下载完成
        _updateProgress(_preparationWeight + _peDownloadWeight + _isoDownloadWeight);
      }
    } else if (data.contains('开始安装')) {
      provider.setStatus(InstallStatus.installing);
      _updateProgress(_preparationWeight + _peDownloadWeight + _isoDownloadWeight);
    } else if (data.contains('Preparation completed')) {
      provider.setStatus(InstallStatus.completed);
      _updateProgress(1.0);
    }

    // 处理百分比进度
    if (data.contains('%')) {
      try {
        final percentStr = RegExp(r'(\d+)%').firstMatch(data)?.group(1);
        if (percentStr != null) {
          final percent = int.parse(percentStr) / 100;
          final currentPhase = provider.status;
          final currentText = data.toLowerCase();
          
          switch (currentPhase) {
            case InstallStatus.downloading:
              if (currentText.contains('boot.wim')) {
                // PE下载进度
                _updateProgress(_preparationWeight + (_peDownloadWeight * percent));
              } else if (currentText.contains('.iso')) {
                // ISO下载进度
                _updateProgress(_preparationWeight + _peDownloadWeight + (_isoDownloadWeight * percent));
              }
              break;
            case InstallStatus.installing:
              if (!_isInstallingPhase) {
                // 只有在非命令执行阶段才处理百分比进度
                if (currentText.contains('dism')) {
                  _updateProgress(_preparationWeight + _peDownloadWeight + _isoDownloadWeight + (_installWeight * 0.3 * percent));
                }
              }
              break;
            default:
              break;
          }
        }
      } catch (_) {}
    }
  }

  void _updateProgress(double targetProgress) {
    if (targetProgress > _lastProgress) {
      _lastProgress = targetProgress;
      provider.setProgress(targetProgress);
    }
  }
} 