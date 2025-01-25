#include <graphics.h>
#include <conio.h>
#include <cmath>
#include <commdlg.h>
#include <cstdio>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <array>
#include <memory>
#include <stdexcept>
#include <fstream>

#include <Shlwapi.h>  // 需要链接 Shlwapi.lib
#include <vector>

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

const int WIDTH = 600;
const int HEIGHT = 400;

// 颜色方案
const COLORREF BACKGROUND_COLOR = RGB(243, 243, 243);
const COLORREF PRIMARY_COLOR = RGB(0, 120, 212);
const COLORREF HOVER_COLOR = RGB(230, 230, 230);
const COLORREF TEXT_COLOR = RGB(51, 51, 51);
const COLORREF HOVER_TEXT_COLOR = RGB(0, 90, 158);

enum Selection { NONE, WIN10, WIN11, CUSTOM };

struct Button {
    RECT rect;
    const char* text;
    bool hovered;
    bool pressed;
    std::chrono::time_point<std::chrono::steady_clock> pressTime;
};

struct RadioOption {
    RECT rect;
    const char* text;
    bool selected;
    bool hovered;
    float animProgress;
};

Selection currentSelection = NONE;
char customFilePath[MAX_PATH] = { 0 };

void DrawModernButton(Button btn) {
    // 计算按下动画偏移量
    float pressProgress = 0.0f;
    if (btn.pressed) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - btn.pressTime
        ).count();
        pressProgress = 1.0f - (std::min)(duration / 150.0f, 1.0f); // 使用括号避免宏冲突
    }

    // 创建动态矩形
    RECT animRect = btn.rect;
    animRect.top += static_cast<int>(4 * pressProgress);
    animRect.bottom += static_cast<int>(4 * pressProgress);

    setfillcolor(btn.pressed ? PRIMARY_COLOR :
        btn.hovered ? HOVER_COLOR : BACKGROUND_COLOR);
    setlinecolor(RGB(200, 200, 200));
    setlinestyle(PS_SOLID, 2);

    // 绘制带偏移的按钮
    roundrect(animRect.left, animRect.top,
        animRect.right, animRect.bottom, 8, 8);

    settextcolor(TEXT_COLOR);
    setbkmode(TRANSPARENT);
    RECT textRect = animRect;
    drawtext(btn.text, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawRadioOption(RadioOption opt) {
    // 先绘制悬停背景
    if (opt.hovered) {
        setfillcolor(HOVER_COLOR);
        solidroundrect(opt.rect.left, opt.rect.top,
            opt.rect.right, opt.rect.bottom, 8, 8);
    }

    // 动画计算
    float scale = 1.0f + opt.animProgress * 0.1f;
    int animSize = static_cast<int>(16 * scale);
    int offset = static_cast<int>((animSize - 16) / 2);

    // 外圈
    setlinecolor(opt.selected ? PRIMARY_COLOR : RGB(150, 150, 150));
    circle(opt.rect.left + 12 - offset,
        (opt.rect.top + opt.rect.bottom) / 2 - offset,
        animSize / 2);

    // 内圈选中状态
    if (opt.selected) {
        setfillcolor(PRIMARY_COLOR);
        solidcircle(opt.rect.left + 12 - offset,
            (opt.rect.top + opt.rect.bottom) / 2 - offset,
            static_cast<int>(8 * (0.5f + opt.animProgress * 0.5f)));
    }

    // 文字
    settextcolor(opt.hovered ? HOVER_TEXT_COLOR : TEXT_COLOR);
    RECT textRect = { opt.rect.left + 32, opt.rect.top,
                    opt.rect.right, opt.rect.bottom };
    drawtext(opt.text, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void UpdateRadioAnimation(RadioOption& opt, bool active) {
    const float ANIM_SPEED = 0.2f;
    if (active) {
        opt.animProgress = (std::min)(1.0f, opt.animProgress + ANIM_SPEED); // 使用括号避免宏冲突
    }
    else {
        opt.animProgress = (std::max)(0.0f, opt.animProgress - ANIM_SPEED); // 使用括号避免宏冲突
    }
}

bool OpenFileDialog(char* path) {
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = GetHWnd();
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "ISO Files (*.iso)\0*.iso\0All Files (*.*)\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    return GetOpenFileName(&ofn);
}

// 函数用于执行命令行并返回输出
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

// 函数用于获取文件的MD5哈希值
std::string getFileMD5(const std::string& filePath) {
    // 构建certutil命令
    std::string command = "certutil -hashfile \"" + filePath + "\" MD5";

    // 执行命令并获取输出
    std::string output = exec(command.c_str());

    // 从输出中提取MD5哈希值
    // MD5哈希值通常在输出的第二行
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

const char* command(const std::string& str) {
    return str.c_str();
}

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

void runCommand(string cmd) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH); // 获取程序完整路径
    PathRemoveFileSpecA(exePath);                // 去除文件名，保留目录
    SetCurrentDirectoryA(exePath);               // 设置工作目录为程序所在目录

    //char currentDir[MAX_PATH];
    //GetCurrentDirectoryA(MAX_PATH, currentDir);
    //std::cout << "[DEBUG] 当前工作目录: " << currentDir << std::endl;
    // 将命令转换为可修改的缓冲区
    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (CreateProcessA(
        NULL,                   // 不直接指定程序路径
        cmdBuffer.data(),       // 命令行字符串（可修改的缓冲区）
        NULL, NULL, FALSE,      // 默认进程属性
        0, NULL, NULL,          // 使用父进程环境和工作目录
        &si, &pi
    )) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        //std::cout << "命令执行成功！" << std::endl;
    }
    else {
        DWORD error = GetLastError();
        std::cerr << "错误：无法执行命令，错误码 " << error << std::endl;
    }
}

int main(int argc, char** argv) {
    system("del /S /Q sources > nul");
    string fileName = argv[0];
    if (argc == 1) {
        FreeConsole();
        initgraph(WIDTH, HEIGHT);
        setbkcolor(BACKGROUND_COLOR);
        cleardevice();

        const int MARGIN = 50;
        const int OPTION_HEIGHT = 40;
        const int SPACING = 10;

        RadioOption options[3] = {
            {{MARGIN, 80, WIDTH - MARGIN, 80 + OPTION_HEIGHT}, "Windows 10", false, false, 0},
            {{MARGIN, 80 + OPTION_HEIGHT + SPACING, WIDTH - MARGIN, 80 + OPTION_HEIGHT * 2 + SPACING}, "Windows 11", false, false, 0},
            {{MARGIN, 80 + OPTION_HEIGHT * 2 + SPACING * 2, WIDTH - MARGIN, 80 + OPTION_HEIGHT * 3 + SPACING * 2}, "自定义 ISO 镜像 （原版镜像）", false, false, 0}
        };

        Button confirmBtn = { {WIDTH / 2 - 60, 280, WIDTH / 2 + 60, 320}, "确定", false, false, {} };

        ExMessage msg;
        BeginBatchDraw();

        while (true) {
            bool needProcessClick = false;

            while (peekmessage(&msg, EX_MOUSE)) {
                for (int i = 0; i < 3; i++) {
                    options[i].hovered = false;
                    if (msg.x >= options[i].rect.left && msg.x <= options[i].rect.right &&
                        msg.y >= options[i].rect.top && msg.y <= options[i].rect.bottom) {
                        options[i].hovered = true;

                        // 修复逻辑：只在成功选择文件后设置选中状态
                        if (msg.message == WM_LBUTTONDOWN) {
                            if (i == 2) { // 自定义选项
                                char tempPath[MAX_PATH] = { 0 };
                                if (OpenFileDialog(tempPath)) {
                                    currentSelection = CUSTOM;
                                    strcpy_s(customFilePath, tempPath);
                                }
                                else {
                                    currentSelection = NONE; // 取消选择
                                    customFilePath[0] = '\0';
                                }
                            }
                            else {
                                currentSelection = static_cast<Selection>(i + 1);
                            }
                        }
                    }
                }

                // 处理确认按钮
                confirmBtn.hovered = (msg.x >= confirmBtn.rect.left &&
                    msg.x <= confirmBtn.rect.right &&
                    msg.y >= confirmBtn.rect.top &&
                    msg.y <= confirmBtn.rect.bottom);

                // 按下事件处理
                if (msg.message == WM_LBUTTONDOWN && confirmBtn.hovered) {
                    confirmBtn.pressed = true;
                    confirmBtn.pressTime = std::chrono::steady_clock::now();
                }

                // 释放事件处理
                if (msg.message == WM_LBUTTONUP) {
                    if (confirmBtn.pressed && confirmBtn.hovered) {
                        needProcessClick = true;
                    }
                    confirmBtn.pressed = false;
                }
            }

            // 更新动画状态
            if (confirmBtn.pressed) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - confirmBtn.pressTime
                ).count();
                if (duration >= 150) {
                    confirmBtn.pressed = false;
                }
            }

            // 处理点击事件（在动画完成后）
            if (needProcessClick) {
                switch (currentSelection) {
                case WIN10:
                    MessageBox(GetHWnd(), "你选择了 Windows 10", "提示", MB_OK);
                    closegraph();
                    system(command(fileName + " -nogui win10"));
                    break;
                case WIN11:
                    MessageBox(GetHWnd(), "你选择了 Windows 11", "提示", MB_OK);
                    closegraph();
                    system(command(fileName + " -nogui win11 %"));
                    break;
                case CUSTOM:
                    if (customFilePath[0]) {
                        char msgText[256];
                        sprintf_s(msgText, "已选择镜像：:\n%s", customFilePath);
                        MessageBox(GetHWnd(), msgText, "提示", MB_OK);
                        closegraph();
                        system(command(fileName + " -nogui " + customFilePath));
                    }
                    else {
                        MessageBox(GetHWnd(), "请先选择一个 ISO 镜像！",
                            "Error", MB_ICONERROR | MB_OK);
                    }
                    break;
                default:
                    MessageBox(GetHWnd(), "请选择一个系统！", "提示", MB_OK);
                }
            }

            // 更新选中状态
            for (int i = 0; i < 3; i++) {
                options[i].selected = (currentSelection == i + 1);
                UpdateRadioAnimation(options[i], options[i].selected);
            }

            cleardevice();

            // 绘制界面元素
            for (auto& opt : options) {
                DrawRadioOption(opt);
            }
            DrawModernButton(confirmBtn);

            // 显示自定义路径
            if (currentSelection == CUSTOM && customFilePath[0]) {
                settextcolor(TEXT_COLOR);
                RECT pathRect = { MARGIN, 220, WIDTH - MARGIN, 260 };
                drawtext(customFilePath, &pathRect, DT_LEFT | DT_VCENTER | DT_WORD_ELLIPSIS | DT_SINGLELINE);
            }

            FlushBatchDraw();
            Sleep(10);
        }

        EndBatchDraw();
        closegraph();
    }

    else {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        system("cls");
        string nogui = "-nogui";
        string choose;
        string wimNum = "4";
        if(argc == 2) choose = "0";
        else {
            choose = argv[2];
        }
        if (argv[1] == nogui && argc == 2) {
            cout << "一键重装系统" << endl << endl;
            cout << "1.Windows 10 22H2" << endl;
            cout << "2.Windows 11 24H2" << endl;
            cout << endl << "请选择：";
            cin>>choose;
            if (choose == "1") {
                choose = "win10";
            }
            else if (choose == "2") {
                choose = "win11";
            }
            else { 
                MessageBox(GetHWnd(), "错误！", "Error", MB_OK);
                exit(0);
            }
        }
        system("cls");

        cout << "你选择了：" << choose << endl << endl;
        
        Sleep(2000);

        if (choose == "win10") {
            string filename = "Win10_22H2_Chinese_Simplified_x64v1.iso";
            while (true) {
                if (!fileExists(filename)) {
                    // 设置文字颜色为红色
                    //SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                    cout << "即将开始下载..." << endl;
                    //SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    system(command("curl -o " + filename + " \"https://software.download.prss.microsoft.com/dbazure/Win10_22H2_Chinese_Simplified_x64v1.iso?t=400c4945-f59a-4cce-9adc-b93e3d5c6ac7&P1=1737746703&P2=601&P3=2&P4=OdnYlpO6dFjP3vXyMsWUhBmWQn%2bTvDScI%2b1yI%2bosq3Cq8FIHcS4xj8jP9lRmUzR08n9b1FmQaiYvyNTk1M2g9m9xQmxEgGzv6Z4X93YpuNZziRwWByt36Pb4l1NXe1TKxB%2bntiY3jzpxVhpDMhW2wbg1QMLvi%2fv2F1PK0H3j2xPZyaCUQAFh3BKLFZJmYUbGenVay0Bf7SGEcKqzHnr7p8b5wlnLHNEvA3j9n3Qgn7ydThEEaGFh7BNWpEsY87gxgsAaGumFqVKmoXSj0YF8CM5SKzkNAF4Lpk9qjoyDdJWSdJ9wYhLl%2fXr7M8aEPxqUKGeGxPUroFmMGznliW83hw%3d%3d\""));
                }
                else {
                    // 设置文字颜色为绿色
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                    cout << "文件已存在！" << endl << endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
                cout << "正在验证镜像MD5..." << endl;
                if (getFileMD5(filename) == "f494b6b9335f86fa72fd541fc75aef43") {
                    // 设置文字颜色为绿色
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                    cout << "MD5验证通过！" << endl << endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    break;
                }
                else {
                    // 设置文字颜色为红色
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                    cout << "MD5验证未通过，即将重新下载..." << endl << endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    system(command("del " + filename));
                }
            }
            //system("del /S /Q sources");
            system(command("tools\\7z.exe x " + filename + " sources\\install.wim"));
        }

        else if (choose == "win11") {
            string filename = "Win11_24H2_Chinese_Simplified_x64.iso";
            while (true) {
                if (!fileExists(filename)) {
                    // 设置文字颜色为红色
                    //SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                    cout << "即将开始下载..." << endl;
                    //SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    system(command("curl -o " + filename + " \"https://software.download.prss.microsoft.com/dbazure/Win11_24H2_Chinese_Simplified_x64.iso?t=42394233-449d-4349-aa17-831bb22261d8&P1=1737745574&P2=601&P3=2&P4=LNJdYj8bnBd6OaLqr3vt94SnYWK9n3hmcZbi1GLQiK65fWH4W3gV%2bB5FBCmOxApmSg8%2bF6qfizGpBK8KiFQTIpUS9%2f9dJ%2f8f88VLGdWDBThsldcVgoAhohw9ZwbEPTnoU%2fhBggeH0ej%2fr9O3bDRhHDXWjYT8wfCOqY8IZ2cTset%2f3O8Qi%2f5wI2ATK5LyH7ksRGFOxFRfiGxnkC7gdQNTKojZNejaWjweRzZVtxFLI1p6fXuMfQyLApYPidpHd6Iv0nVvJGwCZK%2fVeHrXdcj4I%2bhZl05FUzWV4yW7DcaDbs%2bOxG9icq8YY8zZO2PqGwXMx37Eb7YQ%2fmXuCh88Fetbsg%3d%3d\""));
                }
                else {
                    // 设置文字颜色为绿色
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                    cout << "文件已存在！" << endl << endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
                cout << "正在验证镜像MD5..." << endl;
                if (getFileMD5(filename) == "a0ef4099b38e445bb7ee62c28f6a86aa") {
                    // 设置文字颜色为绿色
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                    cout << "MD5验证通过！" << endl << endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    break;
                }
                else {
                    // 设置文字颜色为红色
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                    cout << "MD5验证未通过，即将重新下载..." << endl << endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    system(command("del " + filename));
                }
            }
            //system("del /S /Q sources");
            system(command("tools\\7z.exe x " + filename + " sources\\install.wim"));
        }

        else {
            //cout << "自定义模式" << endl << endl;
            string filepath = choose;
            string cmd = "tools\\7z.exe x \"" + filepath + "\" sources\\install.wim";
            //delInstall();
            runCommand(cmd);

            while (true) {
                system("cls");
                system(command("dism /Get-WimInfo /WimFile:sources\\install.wim"));

                cout << "请选择要安装的系统：";
                cin >> wimNum;
                system("cls");
                cout << "你选择了：" << endl;
                system(command("dism /Get-WimInfo /WimFile:sources\\install.wim /Index:" + wimNum));

                cout << endl << "是否确定？(Y/N)：";
                string YorN;
                cin >> YorN;
                if (YorN == "Y" || YorN == "y") {
                    break;
                }
                else if (YorN == "N" || YorN == "n") {
                    continue;
                }
                else continue;
            }

        }

        system("tools\\Rename.cmd");

        system("tools\\CreatPE.cmd");

        system("tools\\7z.exe x pe\\boot.wim -oB:\\");

        system("copy tools\\script.cmd B:\\");

        system("xcopy sources B:\\sources /s /e /i /h");

        ofstream fout("set.data");

        fout << wimNum << endl;

        fout.close();

        system("copy set.data B:\\");

        system("del /S /Q set.data");

        system("tools\\boot.cmd");

        system("pause > nul");
        
    }
    
    return 0;
}