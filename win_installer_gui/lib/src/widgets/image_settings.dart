import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:file_picker/file_picker.dart';
import '../providers/installer_provider.dart';

class ImageSettings extends StatelessWidget {
  const ImageSettings({super.key});

  Future<void> _pickImageFile(BuildContext context) async {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['iso', 'wim', 'esd'],
    );

    if (result != null) {
      if (context.mounted) {
        context.read<InstallerProvider>().setCustomImagePath(result.files.single.path!);
      }
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
                Expanded(
                  child: TextFormField(
                    readOnly: true,
                    decoration: const InputDecoration(
                      labelText: '镜像文件路径',
                      border: OutlineInputBorder(),
                      hintText: '请选择系统镜像文件',
                    ),
                    controller: TextEditingController(text: provider.customImagePath),
                  ),
                ),
                const SizedBox(width: 8),
                ElevatedButton.icon(
                  onPressed: () => _pickImageFile(context),
                  icon: const Icon(Icons.folder_open),
                  label: const Text('浏览'),
                ),
              ],
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                const Text('镜像索引：'),
                const SizedBox(width: 8),
                SizedBox(
                  width: 100,
                  child: TextFormField(
                    keyboardType: TextInputType.number,
                    decoration: const InputDecoration(
                      border: OutlineInputBorder(),
                      contentPadding: EdgeInsets.symmetric(
                        horizontal: 8,
                        vertical: 0,
                      ),
                    ),
                    initialValue: provider.imageIndex.toString(),
                    onChanged: (value) {
                      final index = int.tryParse(value);
                      if (index != null && index > 0) {
                        provider.setImageIndex(index);
                      }
                    },
                  ),
                ),
              ],
            ),
          ],
        );
      },
    );
  }
} 