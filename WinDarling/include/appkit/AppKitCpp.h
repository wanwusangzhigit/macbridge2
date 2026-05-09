#ifndef APPKIT_CPP_H
#define APPKIT_CPP_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace WinDarling {

// 枚举类型
enum class NSButtonType {
    Momentary = 0,
    Push = 1,
    Toggle = 2,
    Switch = 3,
    Radio = 4,
    Checkbox = 5
};

enum class NSTextAlignment {
    Left = 0,
    Right = 1,
    Center = 2
};

enum class NSProgressIndicatorStyle {
    Bar = 0,
    Spinning = 1
};

// 几何结构
class NSPoint {
public:
    float x, y;
    NSPoint() : x(0), y(0) {}
    NSPoint(float _x, float _y) : x(_x), y(_y) {}
};

class NSSize {
public:
    float width, height;
    NSSize() : width(0), height(0) {}
    NSSize(float _w, float _h) : width(_w), height(_h) {}
};

class NSRect {
public:
    float x, y, width, height;
    NSRect() : x(0), y(0), width(0), height(0) {}
    NSRect(float _x, float _y, float _w, float _h) : x(_x), y(_y), width(_w), height(_h) {}
    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }
};

// NSColor 类
class NSColor {
public:
    float red, green, blue, alpha;
    NSColor() : red(0), green(0), blue(0), alpha(1) {}
    NSColor(float r, float g, float b, float a = 1.0f) 
        : red(r), green(g), blue(b), alpha(a) {}
    
    static NSColor* colorWithRedGreenBlueAlpha(float r, float g, float b, float a);
    static NSColor* blackColor();
    static NSColor* whiteColor();
    static NSColor* redColor();
    static NSColor* blueColor();
    static NSColor* greenColor();
    static NSColor* grayColor();
    static NSColor* lightGrayColor();
    static NSColor* clearColor();
};

// NSView 类 - 基础视图类
class NSView {
public:
    NSView();
    NSView(const NSRect& frame);
    virtual ~NSView();
    
    virtual void drawRect(const NSRect& dirtyRect);
    
    void setFrame(const NSRect& frame);
    NSRect getFrame() const;
    
    void addSubview(NSView* subview);
    void removeFromSuperview();
    NSView* getSuperview() const;
    std::vector<NSView*> getSubviews() const;
    
    void setNeedsDisplay(bool flag);
    
protected:
    NSRect mFrame;
    NSView* mSuperview;
    std::vector<NSView*> mSubviews;
    static uint32_t sViewCounter;
    uint32_t mViewId;
};

// NSButton 类 - 按钮
class NSButton : public NSView {
public:
    NSButton();
    NSButton(const NSRect& frame);
    virtual ~NSButton() override;
    
    static NSButton* buttonWithTitle(const std::string& title);
    
    void setTitle(const std::string& title);
    std::string getTitle() const;
    
    void setButtonType(NSButtonType type);
    NSButtonType getButtonType() const;
    
    void setState(bool state);
    bool getState() const;
    
    void click();
    void performClick();
    
private:
    std::string mTitle;
    NSButtonType mType;
    bool mState;
};

// NSTextField 类 - 文本字段
class NSTextField : public NSView {
public:
    NSTextField();
    NSTextField(const NSRect& frame);
    virtual ~NSTextField() override;
    
    static NSTextField* labelWithString(const std::string& text);
    static NSTextField* textFieldWithString(const std::string& text);
    
    void setStringValue(const std::string& value);
    std::string stringValue() const;
    
    void setEditable(bool editable);
    bool isEditable() const;
    
    void setAlignment(NSTextAlignment alignment);
    NSTextAlignment getAlignment() const;
    
private:
    std::string mStringValue;
    bool mEditable;
    NSTextAlignment mAlignment;
};

// NSTextView 类 - 文本视图
class NSTextView : public NSView {
public:
    NSTextView();
    NSTextView(const NSRect& frame);
    virtual ~NSTextView() override;
    
    void setString(const std::string& str);
    std::string string() const;
    
    void setEditable(bool editable);
    bool isEditable() const;
    
private:
    std::string mString;
    bool mEditable;
};

// NSScrollView 类 - 滚动视图
class NSScrollView : public NSView {
public:
    NSScrollView();
    NSScrollView(const NSRect& frame);
    virtual ~NSScrollView() override;
    
    void setDocumentView(NSView* view);
    NSView* getDocumentView() const;
    
    void setContentSize(NSSize size);
    NSSize getContentSize() const;
    
private:
    NSView* mDocumentView;
    NSSize mContentSize;
};

// NSSlider 类 - 滑块
class NSSlider : public NSView {
public:
    NSSlider();
    NSSlider(const NSRect& frame);
    virtual ~NSSlider() override;
    
    void setMinValue(double min);
    double getMinValue() const;
    
    void setMaxValue(double max);
    double getMaxValue() const;
    
    void setDoubleValue(double value);
    double getDoubleValue() const;
    
    typedef std::function<void(double)> ValueChangedCallback;
    void setValueChangedCallback(ValueChangedCallback callback);
    
private:
    double mMinValue;
    double mMaxValue;
    double mValue;
    ValueChangedCallback mCallback;
};

// NSProgressIndicator 类 - 进度指示器
class NSProgressIndicator : public NSView {
public:
    NSProgressIndicator();
    NSProgressIndicator(const NSRect& frame);
    virtual ~NSProgressIndicator() override;
    
    void setStyle(NSProgressIndicatorStyle style);
    NSProgressIndicatorStyle getStyle() const;
    
    void setDoubleValue(double value);
    double getDoubleValue() const;
    
    void setIndeterminate(bool flag);
    bool isIndeterminate() const;
    
    void startAnimation();
    void stopAnimation();
    bool isAnimating() const;
    
private:
    NSProgressIndicatorStyle mStyle;
    double mValue;
    bool mIndeterminate;
    bool mAnimating;
};

// NSImageView 类 - 图像视图
class NSImageView : public NSView {
public:
    NSImageView();
    NSImageView(const NSRect& frame);
    virtual ~NSImageView() override;
    
    void setImageData(const void* data, size_t size);
    void setImageSize(NSSize size);
    NSSize getImageSize() const;
    
    void setEditable(bool editable);
    bool isEditable() const;
    
private:
    void* mImageData;
    size_t mImageDataSize;
    NSSize mImageSize;
    bool mEditable;
};

// 窗口样式
enum class NSWindowStyleMask {
    Titled = 1 << 0,
    Closable = 1 << 1,
    Miniaturizable = 1 << 2,
    Resizable = 1 << 3
};

// NSWindow 类
class NSWindow {
public:
    NSWindow();
    NSWindow(const NSRect& contentRect, NSWindowStyleMask styleMask);
    virtual ~NSWindow();
    
    void setTitle(const std::string& title);
    std::string getTitle() const;
    
    void setFrame(const NSRect& frame);
    NSRect getFrame() const;
    
    void setContentView(NSView* view);
    NSView* getContentView() const;
    
    void makeKeyAndOrderFront();
    void orderOut();
    void close();
    
    bool isVisible() const;
    void setVisible(bool visible);
    
private:
    std::string mTitle;
    NSRect mFrame;
    NSView* mContentView;
    bool mVisible;
    NSWindowStyleMask mStyleMask;
    static uint32_t sWindowCounter;
    uint32_t mWindowId;
};

// NSApplication 类
class NSApplication {
public:
    static NSApplication* sharedApplication();
    virtual ~NSApplication();
    
    void run();
    void stop();
    void terminate();
    
    void activateIgnoringOtherApps(bool flag);
    
    bool isRunning() const;
    void setRunning(bool running);
    
private:
    NSApplication();
    
    bool mRunning;
    static NSApplication* sSharedApplication;
};

} // namespace WinDarling

#endif // APPKIT_CPP_H
