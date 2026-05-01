#include <iostream>
#include <iomanip>
#include <sstream>
#include "appkit/AppKitCpp.h"
#include "foundation/FoundationCpp.h"

using namespace WinDarling;

// 简单的应用程序类
class MyApp {
public:
    MyApp() : mWindow(nullptr), mContent(nullptr), mSlider(nullptr), mSliderLabel(nullptr), 
              mProgress(nullptr), mButton(nullptr), mTextField(nullptr), mNotesView(nullptr), mImage(nullptr), mCounter(0), mNotificationsReceived(0) {
    }
    
    void setup() {
        std::cout << "========================================" << std::endl;
        std::cout << "  WinDarling Advanced Demo" << std::endl;
        std::cout << "========================================" << std::endl << std::endl;
        
        // 设置用户默认值
        NSUserDefaults* defaults = NSUserDefaults::standardUserDefaults();
        defaults->setStringForKey("WinDarling", "appName");
        defaults->setBoolForKey(true, "darkMode");
        defaults->setIntegerForKey(42, "welcomeMessage");
        std::cout << "✓ NSUserDefaults initialized" << std::endl;
        
        // 设置通知观察
        setupNotifications();
        std::cout << "✓ NSNotificationCenter observers added" << std::endl;
        
        // 设置窗口
        createWindow();
        std::cout << "✓ NSWindow created" << std::endl;
        
        // 设置UI组件
        setupUIComponents();
        std::cout << "✓ UI components setup" << std::endl;
        
        std::cout << std::endl;
    }
    
    void run() {
        // 演示 Foundation 功能
        std::cout << "演示 Foundation 功能" << std::endl;
        std::cout << "--------------------------------" << std::endl;
        
        // 测试文件管理器
        NSFileManager* fm = NSFileManager::defaultManager();
        std::cout << "当前目录: " << fm->currentDirectoryPath() << std::endl;
        std::cout << "是否存在: " << (fm->fileExistsAtPath("/tmp") ? "是" : "否") << std::endl;
        
        // 演示进度
        std::cout << std::endl;
        std::cout << "演示 UI 交互" << std::endl;
        std::cout << "--------------------------------" << std::endl;
        
        // 模拟滑块交互
        std::cout << "滑块初始值: " << 0.5 << std::endl;
        std::cout << "模拟滑块移动: ";
        for (int i = 0; i < 3; ++i) {
            double newValue = 0.3 + (i * 0.35);
            std::cout << std::fixed << std::setprecision(2) << newValue << "  ";
        }
        std::cout << std::endl;
        
        // 模拟按钮点击
        std::cout << std::endl;
        for (int i = 0; i < 5; ++i) {
            simulateButtonClick(i);
        }
        
        // 模拟图片显示
        std::cout << std::endl;
        std::cout << "通知中心演示" << std::endl;
        std::cout << "--------------------------------" << std::endl;
        
        NSNotificationCenter* center = NSNotificationCenter::defaultCenter();
        center->postNotificationName("DataChanged");
        center->postNotificationName("UIClicked");
        center->postNotificationName("UserAction");
        
        std::cout << "收到通知数: " << mNotificationsReceived << std::endl;
        
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "✓ 所有演示完成!" << std::endl;
    }
    
    void cleanup() {
        std::cout << "清理资源中..." << std::endl;
        
        if (mWindow) {
            delete mWindow;
            mWindow = nullptr;
        }
        
        // 移除通知观察
        NSNotificationCenter* center = NSNotificationCenter::defaultCenter();
        center->removeObserver(this);
        
        std::cout << "✓ 资源清理完成" << std::endl;
    }
    
private:
    void setupNotifications() {
        NSNotificationCenter* center = NSNotificationCenter::defaultCenter();
        
        center->addObserver("DataChanged", this, [this](void* obj) {
            this->handleNotification("DataChanged", obj);
        });
        
        center->addObserver("UIClicked", this, [this](void* obj) {
            this->handleNotification("UIClicked", obj);
        });
        
        center->addObserver("UserAction", this, [this](void* obj) {
            this->handleNotification("UserAction", obj);
        });
    }
    
    void createWindow() {
        mWindow = new NSWindow();
        mWindow->setTitle("WinDarling Advanced Demo");
        mWindow->setFrame(NSRect(100, 100, 600, 500));
    }
    
    void setupUIComponents() {
        // 创建内容视图
        mContent = new NSView();
        mWindow->setContentView(mContent);
        
        // 创建滑块
        mSlider = new NSSlider();
        mSlider->setFrame(NSRect(50, 400, 500, 30));
        mSlider->setMinValue(0.0);
        mSlider->setMaxValue(1.0);
        mSlider->setValueChangedCallback([this](double newValue) {
            onSliderChanged(newValue);
        });
        mContent->addSubview(mSlider);
        
        // 创建滑块标签
        mSliderLabel = NSTextField::labelWithString("音量: 50%");
        mSliderLabel->setFrame(NSRect(50, 360, 500, 30));
        mContent->addSubview(mSliderLabel);
        
        // 创建进度条
        mProgress = new NSProgressIndicator();
        mProgress->setFrame(NSRect(50, 300, 500, 30));
        mProgress->setDoubleValue(0.0);
        mContent->addSubview(mProgress);
        
        // 创建按钮
        mButton = NSButton::buttonWithTitle("点击我");
        mButton->setFrame(NSRect(50, 240, 500, 50));
        mContent->addSubview(mButton);
        
        // 创建文本框
        mTextField = NSTextField::textFieldWithString("欢迎使用 WinDarling!");
        mTextField->setFrame(NSRect(50, 180, 500, 40));
        mTextField->setAlignment(NSTextAlignment::Center);
        mContent->addSubview(mTextField);
        
        // 创建文本视图
        mNotesView = new NSTextView(NSRect(50, 100, 500, 60));
        mNotesView->setEditable(true);
        mContent->addSubview(mNotesView);
        
        // 创建图像视图
        mImage = new NSImageView(NSRect(50, 30, 500, 50));
        mContent->addSubview(mImage);
    }
    
    void onSliderChanged(double value) {
        mCounter++;
        
        // 更新进度条
        mProgress->setDoubleValue(value);
        
        // 更新标签
        int percent = static_cast<int>(value * 100);
        std::string labelText = "音量: " + std::to_string(percent) + "%";
        mSliderLabel->setStringValue(labelText);
    }
    
    void simulateButtonClick(int clickNumber) {
        mCounter++;
        mButton->performClick();
        std::cout << "按钮被点击 " << (clickNumber + 1) << " 次" << std::endl;
    }
    
    void handleNotification(const std::string& name, void* object) {
        mNotificationsReceived++;
        std::cout << "收到通知: " << name << std::endl;
    }
    
private:
    NSWindow* mWindow;
    NSView* mContent;
    NSSlider* mSlider;
    NSTextField* mSliderLabel;
    NSProgressIndicator* mProgress;
    NSButton* mButton;
    NSTextField* mTextField;
    NSTextView* mNotesView;
    NSImageView* mImage;
    int mCounter;
    int mNotificationsReceived;
};

int main(int argc, char** argv) {
    try {
        MyApp app;
        app.setup();
        app.run();
        app.cleanup();
        
        std::cout << "演示程序成功完成!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "未知错误!" << std::endl;
        return 1;
    }
}
