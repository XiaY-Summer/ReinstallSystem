#include <iostream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <array>

namespace fs = std::filesystem;

// ������
#define CHECK(condition, message) \
    if (!(condition)) { \
        std::cerr << "[ERROR] " << message << std::endl; \
        exit(EXIT_FAILURE); \
    }

// ϵͳ���ýṹ��
struct Config {
    std::string select_mode;
    std::string image_path;
    int image_index = 1;  // Ĭ��������Windows������1��ʼ��
    bool backup_drive = false;
};

// ִ����������
void ExecuteCommand(const std::string& cmd) {
    std::cout << "[EXEC] " << cmd << std::endl;
    int result = system(cmd.c_str());
    CHECK(result == 0, "Command failed: " + cmd);
}

// ִ�������в��������
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

// ��ȡ�ļ���MD5��ϣ
std::string getFileMD5(const std::string& filePath) {
    // ����certutil����
    std::string command = "certutil -hashfile \"" + filePath + "\" MD5";

    // ִ�������ȡ���
    std::string output = exec(command.c_str());

    // ���������ȡMD5��ϣֵ
    size_t pos = output.find("\n");
    if (pos != std::string::npos) {
        size_t endPos = output.find("\n", pos + 1);
        if (endPos != std::string::npos) {
            std::string md5 = output.substr(pos + 1, endPos - pos - 1);
            // ȥ�����ܵĿհ��ַ�
            md5.erase(md5.find_last_not_of(" \n\r\t") + 1);
            return md5;
        }
    }

    return ""; // ���δ�ҵ�MD5��ϣֵ�����ؿ��ַ���
}

// �ļ������Լ�麯��
bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

//�����ļ�
void downloadAndVerifyFile(
    const std::string& filename,
    const std::string& downloadPath,
    const std::string& expectedMD5
) {
    while (true) {
        // �ļ������Լ���
        if (!fileExists(filename)) {
            std::cout << "������ʼ����..." << std::endl;
            std::string cmd = "curl -o \"" + filename + "\" \"" + downloadPath + "\"";
            std::system(cmd.c_str());
        } else {
            std::cout << "�ļ��Ѵ��ڣ�\n" << std::endl;
        }

        // MD5��֤
        std::cout << "������֤����MD5..." << std::endl;
        std::string actualMD5 = getFileMD5(filename);
        
        if (actualMD5 == expectedMD5) {
            std::cout << "MD5��֤ͨ����\n" << std::endl;
            break;
        } else {
            std::cout << "MD5��֤δͨ����������������...\n" << std::endl;
            std::string delCmd = "del \"" + filename + "\"";  // ɾ��δͨ����֤���ļ�
            std::system(delCmd.c_str());
        }
    }
}

// ��������
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
    
    // ����У��
    CHECK((config.select_mode == "win10" || 
          config.select_mode == "win11" || 
          config.select_mode == "custom"), 
          "Invalid --select value");
    
    if (config.select_mode == "custom") {
        CHECK(!config.image_path.empty(), "--path required for custom mode");
        CHECK(fs::exists(config.image_path), "Image file not found: " + config.image_path);
    } else {
        config.image_index = 4;  // Ԥ��ģʽ�̶�����4
    }
    
    return config;
}

// ����ϵͳ����
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

// ׼������Ŀ¼
void PrepareMountDir() {
    fs::remove_all("mount");
    fs::create_directory("mount");
}

// ����������ע��
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

//���ؾ���
void downloadISO(const Config& config){
    std::string fileName = (config.select_mode == "win10") ? "WIN10.iso" : "WIN11.iso";
    std::string downloadPath = (config.select_mode == "win10") ? "win10���ص�ַ" : "win11���ص�ַ";
    std::string fileMd5 = (config.select_mode == "win10") ? "win10��md5" : "win11��md5";
    downloadAndVerifyFile(fileName,downloadPath,fileMd5);
}
//����PE
void downloadPE(){
    std::string fileName = "pe\\boot.wim";
    std::string downloadPath = "pe��������ص�ַ";
    std::string fileMd5 = "pe��md5";
    downloadAndVerifyFile(fileName,downloadPath,fileMd5);
}


int main(int argc, char* argv[]) {
    // ������ԱȨ��
    CHECK(system("net session >nul 2>&1") == 0, "Require administrator privileges");
    
    // ������ҪĿ¼
    fs::remove_all("sources");
    fs::remove_all("drivers");
    fs::create_directories("sources");
    fs::create_directories("drivers");
    fs::create_directories("pe");
    
    //����PE����
    downloadPE();

    // ��������
    Config config = ParseArguments(argc, argv);

    //���ؾ���
    downloadISO(config);
    
    // ������
    ProcessImage(config);
    
    // ��������
    BackupAndInjectDrivers(config);

    // ִ�г�ʼ���ű�
    ExecuteCommand("tools\\Rename.cmd");
    ExecuteCommand("tools\\CreatPE.cmd");
    
    // �����ļ���PE����
    ExecuteCommand("tools\\7z x pe\\boot.wim -oB:\\");
    ExecuteCommand("xcopy /y sources\\install.wim B:\\sources\\");
    
    // ���������ļ�
    std::ofstream set_data("B:\\set.data");
    set_data << config.image_index;
    set_data.close();
    
    // ���ƽű�
    ExecuteCommand("xcopy /y tools\\script.cmd B:\\");
    ExecuteCommand("xcopy /y tools\\DelPE.cmd B:\\Windows\\System32\\");
    
    // ������PE
    ExecuteCommand("tools\\boot.cmd");
    
    std::cout << "[SUCCESS] Preparation completed. Rebooting..." << std::endl;
    return 0;
}