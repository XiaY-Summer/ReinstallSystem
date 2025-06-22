import 'dart:io';
import 'package:flutter/material.dart';
import 'package:path/path.dart' as path;
import '../services/installer_service.dart';

enum SystemType { win10, win11, custom }
enum InstallStatus { idle, preparing, downloading, installing, completed, error }

class InstallerProvider extends ChangeNotifier {
  SystemType _selectedSystem = SystemType.win10;
  String _customImagePath = '';
  int _imageIndex = 1;
  bool _backupDrivers = false;
  InstallStatus _status = InstallStatus.idle;
  double _progress = 0.0;
  String _currentStep = '';
  String _errorMessage = '';
  bool _isAdminMode = false;
  late final InstallerService _installerService;

  InstallerProvider() {
    _installerService = InstallerService(this);
  }

  // Getters
  SystemType get selectedSystem => _selectedSystem;
  String get customImagePath => _customImagePath;
  int get imageIndex => _imageIndex;
  bool get backupDrivers => _backupDrivers;
  InstallStatus get status => _status;
  double get progress => _progress;
  String get currentStep => _currentStep;
  String get errorMessage => _errorMessage;
  bool get isAdminMode => _isAdminMode;

  // Setters
  void setSystemType(SystemType type) {
    _selectedSystem = type;
    if (type != SystemType.custom) {
      _customImagePath = '';
      _imageIndex = 4; // 预设模式固定索引4
    }
    notifyListeners();
  }

  void setCustomImagePath(String path) {
    _customImagePath = path;
    notifyListeners();
  }

  void setImageIndex(int index) {
    if (index > 0) {
      _imageIndex = index;
      notifyListeners();
    }
  }

  void setBackupDrivers(bool value) {
    _backupDrivers = value;
    notifyListeners();
  }

  void setStatus(InstallStatus newStatus) {
    _status = newStatus;
    notifyListeners();
  }

  void setProgress(double value) {
    _progress = value;
    notifyListeners();
  }

  void setCurrentStep(String step) {
    _currentStep = step;
    notifyListeners();
  }

  void setErrorMessage(String message) {
    _errorMessage = message;
    notifyListeners();
  }

  void setAdminMode(bool value) {
    _isAdminMode = value;
    notifyListeners();
  }

  // 检查管理员权限
  Future<bool> checkAdminPrivileges() async {
    try {
      final result = await Process.run('net', ['session'], runInShell: true);
      _isAdminMode = result.exitCode == 0;
      notifyListeners();
      return _isAdminMode;
    } catch (e) {
      _isAdminMode = false;
      notifyListeners();
      return false;
    }
  }

  // 验证安装配置
  String? validateConfiguration() {
    if (_selectedSystem == SystemType.custom) {
      if (_customImagePath.isEmpty) {
        return '请选择自定义镜像文件';
      }
      if (!File(_customImagePath).existsSync()) {
        return '所选镜像文件不存在';
      }
      final ext = path.extension(_customImagePath).toLowerCase();
      if (!['.iso', '.wim', '.esd'].contains(ext)) {
        return '不支持的镜像文件格式';
      }
    }
    return null;
  }

  // 重置状态
  void reset() {
    _status = InstallStatus.idle;
    _progress = 0.0;
    _currentStep = '';
    _errorMessage = '';
    notifyListeners();
  }

  // 开始安装
  Future<void> startInstallation() async {
    if (!_isAdminMode) {
      setErrorMessage('需要管理员权限');
      return;
    }

    final validation = validateConfiguration();
    if (validation != null) {
      setErrorMessage(validation);
      return;
    }

    try {
      setStatus(InstallStatus.preparing);
      setProgress(0.0);
      setCurrentStep('准备安装环境...');

      await _installerService.runInstaller();
    } catch (e) {
      setStatus(InstallStatus.error);
      setErrorMessage(e.toString());
    }
  }
} 