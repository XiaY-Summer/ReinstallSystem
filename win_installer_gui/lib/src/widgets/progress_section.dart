import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/installer_provider.dart';

class ProgressSection extends StatelessWidget {
  const ProgressSection({super.key});

  Widget _buildStatusIcon(InstallStatus status) {
    switch (status) {
      case InstallStatus.idle:
        return const Icon(Icons.hourglass_empty, color: Colors.grey);
      case InstallStatus.preparing:
      case InstallStatus.downloading:
      case InstallStatus.installing:
        return const SizedBox(
          width: 24,
          height: 24,
          child: CircularProgressIndicator(
            strokeWidth: 2,
          ),
        );
      case InstallStatus.completed:
        return const Icon(Icons.check_circle, color: Colors.green);
      case InstallStatus.error:
        return const Icon(Icons.error, color: Colors.red);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<InstallerProvider>(
      builder: (context, provider, child) {
        return Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                _buildStatusIcon(provider.status),
                const SizedBox(width: 8),
                Expanded(
                  child: Text(
                    provider.currentStep.isEmpty ? '等待开始...' : provider.currentStep,
                    style: const TextStyle(fontSize: 16),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            LinearProgressIndicator(
              value: provider.progress,
              backgroundColor: Colors.grey[200],
              minHeight: 10,
            ),
            const SizedBox(height: 8),
            Text(
              '总进度：${(provider.progress * 100).toStringAsFixed(1)}%',
              style: const TextStyle(
                fontSize: 12,
                color: Colors.grey,
              ),
            ),
            if (provider.errorMessage.isNotEmpty) ...[
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  color: Colors.red[50],
                  borderRadius: BorderRadius.circular(4),
                  border: Border.all(color: Colors.red[200]!),
                ),
                child: Row(
                  children: [
                    const Icon(Icons.error_outline, color: Colors.red),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        provider.errorMessage,
                        style: const TextStyle(color: Colors.red),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ],
        );
      },
    );
  }
} 