#include <iostream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <array>

namespace fs = std::filesystem;

// 错误处理
#define CHECK(condition, message) \
    if (!(condition)) { \
        std::cerr << "[ERROR] " << message << std::endl; \
        exit(EXIT_FAILURE); \
    }

// 系统配置结构体
struct Config {
    std::string select_mode;
    std::string image_path;
    int image_index = 1;  // 默认索引（Windows索引从1开始）
    bool backup_drive = false;
};

// 执行命令并检查结果
void ExecuteCommand(const std::string& cmd) {
    std::cout << "[EXEC] " << cmd << std::endl;
    int result = system(cmd.c_str());
    CHECK(result == 0, "Command failed: " + cmd);
}

// 执行命令行并返回输出
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// 获取文件的MD5哈希
std::string getFileMD5(const std::string& filePath) {
    // 构建certutil命令
    std::string command = "certutil -hashfile \"" + filePath + "\" MD5";

    // 执行命令并获取输出
    std::string output = exec(command.c_str());

    // 从输出中提取MD5哈希值
    size_t pos = output.find("\n");
    if (pos != std::string::npos) {
        size_t endPos = output.find("\n", pos + 1);
        if (endPos != std::string::npos) {
            std::string md5 = output.substr(pos + 1, endPos - pos - 1);
            // 去除可能的空白字符
            md5.erase(md5.find_last_not_of(" \n\r\t") + 1);
            return md5;
        }
    }

    return ""; // 如果未找到MD5哈希值，返回空字符串
}

// 文件存在性检查函数
bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

//下载文件
void downloadAndVerifyFile(
    const std::string& filename,
    const std::string& downloadPath,
    const std::string& expectedMD5
) {
    while (true) {
        // 文件存在性检验
        if (!fileExists(filename)) {
            std::cout << "即将开始下载..." << std::endl;
            std::string cmd = "curl -o \"" + filename + "\" \"" + downloadPath + "\"";
            std::system(cmd.c_str());
        } else {
            std::cout << "文件已存在！\n" << std::endl;
        }

        // MD5验证
        std::cout << "正在验证镜像MD5..." << std::endl;
        std::string actualMD5 = getFileMD5(filename);
        
        if (actualMD5 == expectedMD5) {
            std::cout << "MD5验证通过！\n" << std::endl;
            break;
        } else {
            std::cout << "MD5验证未通过，即将重新下载...\n" << std::endl;
            std::string delCmd = "del \"" + filename + "\"";  // 删除未通过验证的文件
            std::system(delCmd.c_str());
        }
    }
}

// 参数解析
Config ParseArguments(int argc, char* argv[]) {
    Config config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--select") {
            CHECK(i + 1 < argc, "Missing value for --select");
            config.select_mode = argv[++i];
        } else if (arg == "--path") {
            CHECK(i + 1 < argc, "Missing value for --path");
            config.image_path = argv[++i];
        } else if (arg == "--set") {
            CHECK(i + 1 < argc, "Missing value for --set");
            config.image_index = std::stoi(argv[++i]);
            CHECK(config.image_index >= 1, "--set value must be >= 1");
        } else if (arg == "--backupdrive") {
            CHECK(i + 1 < argc, "Missing value for --backupdrive");
            std::string value = argv[++i];
            config.backup_drive = (value == "true");
        }
    }
    
    // 参数校验
    CHECK((config.select_mode == "win10" || 
          config.select_mode == "win11" || 
          config.select_mode == "custom"), 
          "Invalid --select value");
    
    if (config.select_mode == "custom") {
        CHECK(!config.image_path.empty(), "--path required for custom mode");
        CHECK(fs::exists(config.image_path), "Image file not found: " + config.image_path);
    } else {
        config.image_index = 4;  // 预置模式固定索引4
    }
    
    return config;
}

// 处理系统镜像
void ProcessImage(const Config& config) {
    if (config.select_mode != "custom") {
        std::string source_iso = (config.select_mode == "win10") ? "WIN10.iso" : "WIN11.iso";
        CHECK(fs::exists(source_iso), "Missing ISO file: " + source_iso);
        ExecuteCommand("tools\\7z x \"" + source_iso + "\" sources/install.wim -y");
    } else {
        std::string ext = fs::path(config.image_path).extension().string();
        if (ext == ".iso") {
            ExecuteCommand("tools\\7z x \"" + config.image_path + "\" sources/install.wim -y");
        } else if (ext == ".wim") {
            fs::copy_file(config.image_path, "sources/install.wim", fs::copy_options::overwrite_existing);
        } else if (ext == ".esd") {
            ExecuteCommand("dism /export-image /SourceImageFile:\"" + config.image_path + 
                          "\" /SourceIndex:" + std::to_string(config.image_index) + 
                          " /DestinationImageFile:\"sources\\install.wim\" /Compress:max");
        }
    }
    CHECK(fs::exists("sources/install.wim"), "Failed to generate install.wim");
}

// 准备挂载目录
void PrepareMountDir() {
    fs::remove_all("mount");
    fs::create_directory("mount");
}

// 驱动备份与注入
void BackupAndInjectDrivers(const Config& config) {
    if (config.backup_drive) {
        ExecuteCommand("dism /online /export-driver /destination:drivers");
        PrepareMountDir();
        ExecuteCommand("dism /mount-wim /wimfile:\"sources\\install.wim\" /index:" + 
                      std::to_string(config.image_index) + " /mountdir:mount");
        ExecuteCommand("dism /image:mount /add-driver /driver:drivers /recurse");
        ExecuteCommand("dism /unmount-wim /mountdir:mount /commit");
    }
}

//下载镜像
void downloadISO(const Config& config){
    std::string fileName = (config.select_mode == "win10") ? "WIN10.iso" : "WIN11.iso";
    std::string downloadPath = (config.select_mode == "win10") ? "win10下载地址" : "win11下载地址";
    std::string fileMd5 = (config.select_mode == "win10") ? "win10的md5" : "win11的md5";
    downloadAndVerifyFile(fileName,downloadPath,fileMd5);
}
//下载PE
void downloadPE(){
    std::string fileName = "pe\\boot.wim";
    std::string downloadPath = "pe镜像的下载地址";
    std::string fileMd5 = "pe的md5";
    downloadAndVerifyFile(fileName,downloadPath,fileMd5);
}


int main(int argc, char* argv[]) {
    // 检查管理员权限
    CHECK(system("net session >nul 2>&1") == 0, "Require administrator privileges");
    
    // 创建必要目录
    fs::remove_all("sources");
    fs::remove_all("drivers");
    fs::create_directories("sources");
    fs::create_directories("drivers");
    fs::create_directories("pe");
    
    //下载PE镜像
    downloadPE();

    // 解析参数
    Config config = ParseArguments(argc, argv);

    //下载镜像
    downloadISO(config);
    
    // 处理镜像
    ProcessImage(config);
    
    // 驱动操作
    BackupAndInjectDrivers(config);

    // 执行初始化脚本
    ExecuteCommand("tools\\Rename.cmd");
    ExecuteCommand("tools\\CreatPE.cmd");
    
    // 复制文件到PE分区
    ExecuteCommand("tools\\7z x pe\\boot.wim -oB:\\");
    ExecuteCommand("xcopy /y sources\\install.wim B:\\sources\\");
    
    // 生成配置文件
    std::ofstream set_data("B:\\set.data");
    set_data << config.image_index;
    set_data.close();
    
    // 复制脚本
    ExecuteCommand("xcopy /y tools\\script.cmd B:\\");
    ExecuteCommand("xcopy /y tools\\DelPE.cmd B:\\Windows\\System32\\");
    
    // 重启到PE
    ExecuteCommand("tools\\boot.cmd");
    
    std::cout << "[SUCCESS] Preparation completed. Rebooting..." << std::endl;
    return 0;
}