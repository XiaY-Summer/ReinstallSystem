# Windows 系统重装工具 GUI

这是一个基于 Flutter 开发的 Windows 系统重装工具图形界面，它为 WinInstaller.exe 提供了一个现代化、用户友好的操作界面。

## 功能特点

- 支持安装 Windows 10 和 Windows 11
- 支持自定义系统镜像（ISO/WIM/ESD）
- 自动备份和注入系统驱动
- 实时显示安装进度
- 自动检查管理员权限
- 支持断点续传
- MD5 校验保证文件完整性

## 系统要求

- Windows 10/11 操作系统
- 管理员权限
- 稳定的网络连接（用于下载系统镜像）
- 足够的磁盘空间

## 开发环境设置

1. 安装 Flutter SDK
2. 安装 Visual Studio（用于 Windows 开发）
3. 克隆项目代码
4. 运行以下命令安装依赖：
   ```bash
   flutter pub get
   ```

## 构建应用

```bash
flutter build windows
```

生成的可执行文件将位于 `build/windows/runner/Release` 目录下。

## 使用说明

1. 以管理员身份运行程序
2. 选择要安装的系统类型：
   - Windows 10
   - Windows 11
   - 自定义镜像
3. 如果选择自定义镜像，请选择系统镜像文件
4. 选择是否备份并注入当前系统驱动
5. 点击"开始安装"按钮
6. 等待安装完成，系统将自动重启

## 注意事项

- 安装前请备份重要数据
- 确保电脑接通电源
- 保持网络连接稳定
- 不要中断安装过程
- 如果使用自定义镜像，请确保镜像文件完整且未损坏

## 依赖项

- provider: ^6.1.1
- file_picker: ^6.1.1
- path: ^1.8.3
- window_manager: ^0.3.7
- win32: ^5.1.1

## 许可证

MIT License
