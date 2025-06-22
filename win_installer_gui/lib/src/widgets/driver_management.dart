import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/installer_provider.dart';

class DriverManagement extends StatelessWidget {
  const DriverManagement({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<InstallerProvider>(
      builder: (context, provider, child) {
        return Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            SwitchListTile(
              title: const Text('备份并注入当前系统驱动'),
              subtitle: const Text('建议开启此选项以确保系统安装后驱动完整'),
              value: provider.backupDrivers,
              onChanged: (bool value) {
                provider.setBackupDrivers(value);
              },
            ),
            if (provider.backupDrivers) ...[
              const SizedBox(height: 8),
              const Padding(
                padding: EdgeInsets.symmetric(horizontal: 16),
                child: Text(
                  '驱动将保存在 ./drivers 目录下',
                  style: TextStyle(
                    color: Colors.grey,
                    fontSize: 12,
                  ),
                ),
              ),
            ],
          ],
        );
      },
    );
  }
} 