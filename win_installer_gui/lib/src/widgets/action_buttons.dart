import 'dart:io';
import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/installer_provider.dart';
import '../providers/mouse_provider.dart';

class ActionButtons extends StatefulWidget {
  const ActionButtons({super.key});

  @override
  State<ActionButtons> createState() => _ActionButtonsState();
}

class _ActionButtonsState extends State<ActionButtons> {
  Timer? _countdownTimer;
  int _countdown = 10;
  bool _isCountingDown = false;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<MouseProvider>().addMouseListener(_handleMouseMove);
    });
  }

  @override
  void dispose() {
    _countdownTimer?.cancel();
    context.read<MouseProvider>().removeMouseListener(_handleMouseMove);
    super.dispose();
  }

  void _startCountdown() {
    setState(() {
      _isCountingDown = true;
      _countdown = 10;
    });

    _countdownTimer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (mounted) {
        setState(() {
          _countdown--;
          if (_countdown <= 0) {
            timer.cancel();
            _handleRestart();
          }
        });
      }
    });
  }

  void _cancelCountdown() {
    _countdownTimer?.cancel();
    if (mounted) {
      setState(() {
        _isCountingDown = false;
      });
    }
  }

  void _handleMouseMove() {
    if (_isCountingDown) {
      _cancelCountdown();
    }
  }

  Future<bool> _showConfirmDialog(BuildContext context) async {
    return await showDialog<bool>(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text('确认开始安装'),
          content: const Text(
            '即将开始系统重装过程，请确保：\n'
            '1. 已备份重要数据\n'
            '2. 电脑接通电源\n'
            '3. 网络连接正常\n\n'
            '是否继续？',
          ),
          actions: <Widget>[
            TextButton(
              child: const Text('取消'),
              onPressed: () => Navigator.of(context).pop(false),
            ),
            TextButton(
              child: const Text('继续'),
              onPressed: () => Navigator.of(context).pop(true),
            ),
          ],
        );
      },
    ) ?? false;
  }

  Future<void> _handleStartInstall(BuildContext context) async {
    final provider = context.read<InstallerProvider>();
    
    if (!provider.isAdminMode) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('请以管理员身份运行程序'),
          backgroundColor: Colors.red,
        ),
      );
      return;
    }

    final validation = provider.validateConfiguration();
    if (validation != null) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(validation),
          backgroundColor: Colors.red,
        ),
      );
      return;
    }

    final confirmed = await _showConfirmDialog(context);
    if (confirmed) {
      provider.startInstallation();
    }
  }

  Future<void> _handleRestart() async {
    try {
      await Process.run('shutdown', ['/r', '/t', '0'], runInShell: true);
    } catch (e) {
      debugPrint('重启失败: $e');
    }
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<InstallerProvider>(
      builder: (context, provider, child) {
        final bool isProcessing = provider.status == InstallStatus.preparing ||
            provider.status == InstallStatus.downloading ||
            provider.status == InstallStatus.installing;

        final bool isCompleted = provider.status == InstallStatus.completed;

        // 当安装完成时，自动开始倒计时
        if (isCompleted && !_isCountingDown && _countdownTimer == null) {
          _startCountdown();
        }

        return Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            if (!isCompleted) ...[
              ElevatedButton.icon(
                onPressed: isProcessing ? null : () => _handleStartInstall(context),
                style: ElevatedButton.styleFrom(
                  backgroundColor: Colors.green,
                  foregroundColor: Colors.white,
                  padding: const EdgeInsets.symmetric(
                    horizontal: 32,
                    vertical: 16,
                  ),
                ),
                icon: const Icon(Icons.play_arrow),
                label: const Text('开始安装'),
              ),
            ] else
              ElevatedButton.icon(
                onPressed: _handleRestart,
                style: ElevatedButton.styleFrom(
                  backgroundColor: Colors.blue,
                  foregroundColor: Colors.white,
                  padding: const EdgeInsets.symmetric(
                    horizontal: 32,
                    vertical: 16,
                  ),
                ),
                icon: const Icon(Icons.restart_alt),
                label: Text(_isCountingDown ? '立即重启($_countdown)' : '立即重启'),
              ),
          ],
        );
      },
    );
  }
} 