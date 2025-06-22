import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/installer_provider.dart';

class SystemSelection extends StatelessWidget {
  const SystemSelection({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<InstallerProvider>(
      builder: (context, provider, child) {
        return Column(
          children: [
            RadioListTile<SystemType>(
              title: const Text('Windows 10'),
              value: SystemType.win10,
              groupValue: provider.selectedSystem,
              onChanged: (SystemType? value) {
                if (value != null) {
                  provider.setSystemType(value);
                }
              },
            ),
            RadioListTile<SystemType>(
              title: const Text('Windows 11'),
              value: SystemType.win11,
              groupValue: provider.selectedSystem,
              onChanged: (SystemType? value) {
                if (value != null) {
                  provider.setSystemType(value);
                }
              },
            ),
            RadioListTile<SystemType>(
              title: const Text('自定义镜像'),
              value: SystemType.custom,
              groupValue: provider.selectedSystem,
              onChanged: (SystemType? value) {
                if (value != null) {
                  provider.setSystemType(value);
                }
              },
            ),
          ],
        );
      },
    );
  }
} 