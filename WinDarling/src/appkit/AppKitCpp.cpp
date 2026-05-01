#include "appkit/AppKitCpp.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace WinDarling {

// 静态成员初始化
uint32_t NSView::sViewCounter = 0;
uint32_t NSWindow::sWindowCounter = 0;
NSApplication* NSApplication::sSharedApplication = nullptr;

// NSColor 实现
NSColor* NSColor::colorWithRedGreenBlueAlpha(float r, float g, float b, float a) {
    return new NSColor(r, g, b, a);
}

NSColor* NSColor::blackColor() {
    return new NSColor(0.0f, 0.0f, 0.0f, 1.0f);
}

NSColor* NSColor::whiteColor() {
    return new NSColor(1.0f, 1.0f, 1.0f, 1.0f);
}

NSColor* NSColor::redColor() {
    return new NSColor(1.0f, 0.0f, 0.0f, 1.0f);
}

NSColor* NSColor::blueColor() {
    return new NSColor(0.0f, 0.0f, 1.0f, 1.0f);
}

NSColor* NSColor::greenColor() {
    return new NSColor(0.0f, 1.0f, 0.0f, 1.0f);
}

NSColor* NSColor::grayColor() {
    return new NSColor(0.5f, 0.5f, 0.5f, 1.0f);
}

NSColor* NSColor::lightGrayColor() {
    return new NSColor(0.75f, 0.75f, 0.75f, 1.0f);
}

NSColor* NSColor::clearColor() {
    return new NSColor(0.0f, 0.0f, 0.0f, 0.0f);
}

// NSView 实现
NSView::NSView() 
    : mFrame(), mSuperview(nullptr), mViewId(++sViewCounter) {
}

NSView::NSView(const NSRect& frame) 
    : mFrame(frame), mSuperview(nullptr), mViewId(++sViewCounter) {
}

NSView::~NSView() {
    for (auto subview : mSubviews) {
        delete subview;
    }
}

void NSView::drawRect(const NSRect& dirtyRect) {
}

void NSView::setFrame(const NSRect& frame) {
    mFrame = frame;
}

NSRect NSView::getFrame() const {
    return mFrame;
}

void NSView::addSubview(NSView* subview) {
    if (subview) {
        subview->mSuperview = this;
        mSubviews.push_back(subview);
    }
}

void NSView::removeFromSuperview() {
    if (mSuperview) {
        auto& subviews = mSuperview->mSubviews;
        auto it = std::find(subviews.begin(), subviews.end(), this);
        if (it != subviews.end()) {
            subviews.erase(it);
            mSuperview = nullptr;
        }
    }
}

NSView* NSView::getSuperview() const {
    return mSuperview;
}

std::vector<NSView*> NSView::getSubviews() const {
    return mSubviews;
}

void NSView::setNeedsDisplay(bool flag) {
}

// NSButton 实现
NSButton::NSButton() 
    : NSView(), mType(NSButtonType::Push), mState(false) {
}

NSButton::NSButton(const NSRect& frame) 
    : NSView(frame), mType(NSButtonType::Push), mState(false) {
}

NSButton::~NSButton() {
}

NSButton* NSButton::buttonWithTitle(const std::string& title) {
    NSButton* btn = new NSButton();
    btn->mTitle = title;
    return btn;
}

void NSButton::setTitle(const std::string& title) {
    mTitle = title;
}

std::string NSButton::getTitle() const {
    return mTitle;
}

void NSButton::setButtonType(NSButtonType type) {
    mType = type;
}

NSButtonType NSButton::getButtonType() const {
    return mType;
}

void NSButton::setState(bool state) {
    mState = state;
}

bool NSButton::getState() const {
    return mState;
}

void NSButton::click() {
    performClick();
}

void NSButton::performClick() {
    mState = !mState;
}

// NSTextField 实现
NSTextField::NSTextField() 
    : NSView(), mEditable(false), mAlignment(NSTextAlignment::Left) {
}

NSTextField::NSTextField(const NSRect& frame) 
    : NSView(frame), mEditable(false), mAlignment(NSTextAlignment::Left) {
}

NSTextField::~NSTextField() {
}

NSTextField* NSTextField::labelWithString(const std::string& text) {
    NSTextField* field = new NSTextField();
    field->mStringValue = text;
    field->mEditable = false;
    return field;
}

NSTextField* NSTextField::textFieldWithString(const std::string& text) {
    NSTextField* field = new NSTextField();
    field->mStringValue = text;
    field->mEditable = true;
    return field;
}

void NSTextField::setStringValue(const std::string& value) {
    mStringValue = value;
}

std::string NSTextField::stringValue() const {
    return mStringValue;
}

void NSTextField::setEditable(bool editable) {
    mEditable = editable;
}

bool NSTextField::isEditable() const {
    return mEditable;
}

void NSTextField::setAlignment(NSTextAlignment alignment) {
    mAlignment = alignment;
}

NSTextAlignment NSTextField::getAlignment() const {
    return mAlignment;
}

// NSTextView 实现
NSTextView::NSTextView() 
    : NSView(), mEditable(true) {
}

NSTextView::NSTextView(const NSRect& frame) 
    : NSView(frame), mEditable(true) {
}

NSTextView::~NSTextView() {
}

void NSTextView::setString(const std::string& str) {
    mString = str;
}

std::string NSTextView::string() const {
    return mString;
}

void NSTextView::setEditable(bool editable) {
    mEditable = editable;
}

bool NSTextView::isEditable() const {
    return mEditable;
}

// NSScrollView 实现
NSScrollView::NSScrollView() 
    : NSView(), mDocumentView(nullptr) {
}

NSScrollView::NSScrollView(const NSRect& frame) 
    : NSView(frame), mDocumentView(nullptr) {
}

NSScrollView::~NSScrollView() {
}

void NSScrollView::setDocumentView(NSView* view) {
    mDocumentView = view;
}

NSView* NSScrollView::getDocumentView() const {
    return mDocumentView;
}

void NSScrollView::setContentSize(NSSize size) {
    mContentSize = size;
}

NSSize NSScrollView::getContentSize() const {
    return mContentSize;
}

// NSSlider 实现
NSSlider::NSSlider() 
    : NSView(), mMinValue(0.0), mMaxValue(1.0), mValue(0.5) {
}

NSSlider::NSSlider(const NSRect& frame) 
    : NSView(frame), mMinValue(0.0), mMaxValue(1.0), mValue(0.5) {
}

NSSlider::~NSSlider() {
}

void NSSlider::setMinValue(double min) {
    mMinValue = min;
    if (mValue < mMinValue) {
        mValue = mMinValue;
    }
}

double NSSlider::getMinValue() const {
    return mMinValue;
}

void NSSlider::setMaxValue(double max) {
    mMaxValue = max;
    if (mValue > mMaxValue) {
        mValue = mMaxValue;
    }
}

double NSSlider::getMaxValue() const {
    return mMaxValue;
}

void NSSlider::setDoubleValue(double value) {
    if (value < mMinValue) value = mMinValue;
    if (value > mMaxValue) value = mMaxValue;
    mValue = value;
    if (mCallback) {
        mCallback(mValue);
    }
}

double NSSlider::getDoubleValue() const {
    return mValue;
}

void NSSlider::setValueChangedCallback(ValueChangedCallback callback) {
    mCallback = callback;
}

// NSProgressIndicator 实现
NSProgressIndicator::NSProgressIndicator() 
    : NSView(), mStyle(NSProgressIndicatorStyle::Bar), 
      mValue(0.0), mIndeterminate(false), mAnimating(false) {
}

NSProgressIndicator::NSProgressIndicator(const NSRect& frame) 
    : NSView(frame), mStyle(NSProgressIndicatorStyle::Bar), 
      mValue(0.0), mIndeterminate(false), mAnimating(false) {
}

NSProgressIndicator::~NSProgressIndicator() {
}

void NSProgressIndicator::setStyle(NSProgressIndicatorStyle style) {
    mStyle = style;
}

NSProgressIndicatorStyle NSProgressIndicator::getStyle() const {
    return mStyle;
}

void NSProgressIndicator::setDoubleValue(double value) {
    mValue = value;
}

double NSProgressIndicator::getDoubleValue() const {
    return mValue;
}

void NSProgressIndicator::setIndeterminate(bool flag) {
    mIndeterminate = flag;
}

bool NSProgressIndicator::isIndeterminate() const {
    return mIndeterminate;
}

void NSProgressIndicator::startAnimation() {
    mAnimating = true;
}

void NSProgressIndicator::stopAnimation() {
    mAnimating = false;
}

bool NSProgressIndicator::isAnimating() const {
    return mAnimating;
}

// NSImageView 实现
NSImageView::NSImageView() 
    : NSView(), mImageData(nullptr), mImageDataSize(0), 
      mImageSize(), mEditable(false) {
}

NSImageView::NSImageView(const NSRect& frame) 
    : NSView(frame), mImageData(nullptr), mImageDataSize(0), 
      mImageSize(), mEditable(false) {
}

NSImageView::~NSImageView() {
    if (mImageData) {
        delete[] static_cast<char*>(mImageData);
    }
}

void NSImageView::setImageData(const void* data, size_t size) {
    if (mImageData) {
        delete[] static_cast<char*>(mImageData);
    }
    if (data && size > 0) {
        mImageData = new char[size];
        std::memcpy(mImageData, data, size);
        mImageDataSize = size;
    } else {
        mImageData = nullptr;
        mImageDataSize = 0;
    }
}

void NSImageView::setImageSize(NSSize size) {
    mImageSize = size;
}

NSSize NSImageView::getImageSize() const {
    return mImageSize;
}

void NSImageView::setEditable(bool editable) {
    mEditable = editable;
}

bool NSImageView::isEditable() const {
    return mEditable;
}

// NSWindow 实现
NSWindow::NSWindow() 
    : mFrame(), mContentView(nullptr), mVisible(false), 
      mStyleMask(NSWindowStyleMask::Titled), mWindowId(++sWindowCounter) {
}

NSWindow::NSWindow(const NSRect& contentRect, NSWindowStyleMask styleMask) 
    : mFrame(contentRect), mContentView(nullptr), mVisible(false), 
      mStyleMask(styleMask), mWindowId(++sWindowCounter) {
}

NSWindow::~NSWindow() {
    if (mContentView) {
        delete mContentView;
    }
}

void NSWindow::setTitle(const std::string& title) {
    mTitle = title;
}

std::string NSWindow::getTitle() const {
    return mTitle;
}

void NSWindow::setFrame(const NSRect& frame) {
    mFrame = frame;
}

NSRect NSWindow::getFrame() const {
    return mFrame;
}

void NSWindow::setContentView(NSView* view) {
    if (mContentView) {
        delete mContentView;
    }
    mContentView = view;
}

NSView* NSWindow::getContentView() const {
    return mContentView;
}

void NSWindow::makeKeyAndOrderFront() {
    mVisible = true;
}

void NSWindow::orderOut() {
    mVisible = false;
}

void NSWindow::close() {
    mVisible = false;
}

bool NSWindow::isVisible() const {
    return mVisible;
}

void NSWindow::setVisible(bool visible) {
    mVisible = visible;
}

// NSApplication 实现
NSApplication::NSApplication() : mRunning(false) {
}

NSApplication::~NSApplication() {
}

NSApplication* NSApplication::sharedApplication() {
    if (!sSharedApplication) {
        sSharedApplication = new NSApplication();
    }
    return sSharedApplication;
}

void NSApplication::run() {
    mRunning = true;
}

void NSApplication::stop() {
    mRunning = false;
}

void NSApplication::terminate() {
    mRunning = false;
}

void NSApplication::activateIgnoringOtherApps(bool flag) {
}

bool NSApplication::isRunning() const {
    return mRunning;
}

void NSApplication::setRunning(bool running) {
    mRunning = running;
}

} // namespace WinDarling
