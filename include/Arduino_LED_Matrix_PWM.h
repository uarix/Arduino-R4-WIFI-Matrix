#pragma once  

#include "Arduino.h"  
#include "FspTimer.h"  
#include "gallery.h"  

#define NUM_LEDS    96  
#define PWM_MAX     255  // PWM 最大值  

#if __has_include("ArduinoGraphics.h")  
#include <ArduinoGraphics.h>  
#define MATRIX_WITH_ARDUINOGRAPHICS  
#endif  

static const int pin_zero_index = 28;  

static const uint8_t pins[][2] = {  
  { 7, 3 }, // 0  
  { 3, 7 },  
  { 7, 4 },  
  { 4, 7 },  
  { 3, 4 },  
  { 4, 3 },  
  { 7, 8 },  
  { 8, 7 },  
  { 3, 8 },  
  { 8, 3 },  
  { 4, 8 }, // 10  
  { 8, 4 },  
  { 7, 0 },  
  { 0, 7 },  
  { 3, 0 },  
  { 0, 3 },  
  { 4, 0 },  
  { 0, 4 },  
  { 8, 0 },  
  { 0, 8 },  
  { 7, 6 }, // 20  
  { 6, 7 },  
  { 3, 6 },  
  { 6, 3 },  
  { 4, 6 },  
  { 6, 4 },  
  { 8, 6 },  
  { 6, 8 },  
  { 0, 6 },  
  { 6, 0 },  
  { 7, 5 }, // 30  
  { 5, 7 },  
  { 3, 5 },  
  { 5, 3 },  
  { 4, 5 },  
  { 5, 4 },  
  { 8, 5 },  
  { 5, 8 },  
  { 0, 5 },  
  { 5, 0 },  
  { 6, 5 }, // 40  
  { 5, 6 },  
  { 7, 1 },  
  { 1, 7 },  
  { 3, 1 },  
  { 1, 3 },  
  { 4, 1 },  
  { 1, 4 },  
  { 8, 1 },  
  { 1, 8 },  
  { 0, 1 }, // 50  
  { 1, 0 },  
  { 6, 1 },  
  { 1, 6 },  
  { 5, 1 },  
  { 1, 5 },  
  { 7, 2 },  
  { 2, 7 },  
  { 3, 2 },  
  { 2, 3 },  
  { 4, 2 },  
  { 2, 4 },  
  { 8, 2 },  
  { 2, 8 },  
  { 0, 2 },  
  { 2, 0 },  
  { 6, 2 },  
  { 2, 6 },  
  { 5, 2 },  
  { 2, 5 },  
  { 1, 2 },  
  { 2, 1 },  
  { 7, 10 },  
  { 10, 7 },  
  { 3, 10 },  
  { 10, 3 },  
  { 4, 10 },  
  { 10, 4 },  
  { 8, 10 },  
  { 10, 8 },  
  { 0, 10 },  
  { 10, 0 },  
  { 6, 10 },  
  { 10, 6 },  
  { 5, 10 },  
  { 10, 5 },  
  { 1, 10 },  
  { 10, 1 },  
  { 2, 10 },  
  { 10, 2 },  
  { 7, 9 },  
  { 9, 7 },  
  { 3, 9 },  
  { 9, 3 },  
  { 4, 9 },  
  { 9, 4 },  
};  
#define LED_MATRIX_PORT0_MASK       ((1 << 3) | (1 << 4) | (1 << 11) | (1 << 12) | (1 << 13) | (1 << 15))  
#define LED_MATRIX_PORT2_MASK       ((1 << 4) | (1 << 5) | (1 << 6) | (1 << 12) | (1 << 13))  

static void turnLed(int idx, bool on) {  
  R_PORT0->PCNTR1 &= ~((uint32_t) LED_MATRIX_PORT0_MASK);  
  R_PORT2->PCNTR1 &= ~((uint32_t) LED_MATRIX_PORT2_MASK);  

  if (on) {  
    bsp_io_port_pin_t pin_a = g_pin_cfg[pins[idx][0] + pin_zero_index].pin;  
    R_PFS->PORT[pin_a >> 8].PIN[pin_a & 0xFF].PmnPFS =  
      IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_HIGH;  

    bsp_io_port_pin_t pin_c = g_pin_cfg[pins[idx][1] + pin_zero_index].pin;  
    R_PFS->PORT[pin_c >> 8].PIN[pin_c & 0xFF].PmnPFS =  
      IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_LOW;  
  }  
}  

// 修改宏定义  
#define loadSequence(frames) loadWrapper((const uint8_t*)frames, sizeof(frames) / NUM_LEDS)  
#define renderBitmap(bitmap, rows, columns)     loadPixels(&bitmap[0][0], rows*columns)  
#define endTextAnimation(scrollDirection, anim) endTextToAnimationBuffer(scrollDirection, anim ## _buf, sizeof(anim ## _buf), anim ## _buf_used)  
#define loadTextAnimationSequence(anim)         loadWrapper(anim ## _buf, anim ## _buf_used)  

static uint8_t __attribute__((aligned)) framebuffer[NUM_LEDS];  

class ArduinoLEDMatrix  
#ifdef MATRIX_WITH_ARDUINOGRAPHICS  
    : public ArduinoGraphics  
#endif  
     {  

public:  
    ArduinoLEDMatrix()  
    #ifdef MATRIX_WITH_ARDUINOGRAPHICS  
        : ArduinoGraphics(canvasWidth, canvasHeight)  
    #endif  
    {}  
    // 自动滚动  
    void autoscroll(uint32_t interval_ms) {  
        _interval = interval_ms;  
    }  
    void on(size_t pin) {  
        framebuffer[pin] = PWM_MAX;  // 设置亮度为最大值  
    }  
    void off(size_t pin) {  
        framebuffer[pin] = 0;  // 设置亮度为 0（关闭）  
    }  
    int begin() {  
        bool rv = true;  
        uint8_t type;  //0: GPT_TIMER (32 BIT), 1: AGT_TIMER (16 BIT)
        int8_t ch = FspTimer::get_available_timer(type);  
        if(ch == -1) {  
            return false;  
        }  
        rv &= _ledTimer.begin(TIMER_MODE_PERIODIC, type, ch, 65535, 50.0, turnOnLedISR, this);  
        rv &= _ledTimer.setup_overflow_irq();  
        rv &= _ledTimer.open();  
        rv &= _ledTimer.start();  
        return rv;  
    }  
    void next() {  
        const uint8_t* framePtr = _frames + (_currentFrame * NUM_LEDS);  
        memcpy(framebuffer, framePtr, NUM_LEDS); 
        _currentFrame = (_currentFrame + 1) % _framesCount;  
        if(_currentFrame == 0){  
            if(!_loop){  
                _interval = 0;  
            }  
            if(_callBack != nullptr){  
                _callBack();  
            }  
            _sequenceDone = true;  
        }  
    }  
    // 加载一帧亮度数据  
    void loadFrame(const uint8_t buffer[NUM_LEDS]){  
        loadWrapper(buffer, 1);  
        next();  
        _interval = 0;  
    }  
    void renderFrame(uint8_t frameNumber){  
        _currentFrame = frameNumber % _framesCount;  
        next();  
        _interval = 0;  
    }  
    void play(bool loop = false){  
        _loop = loop;  
        _sequenceDone = false;  
        next();  
    }  
    bool sequenceDone(){  
        if(_sequenceDone){  
            _sequenceDone = false;  
            return true;  
        }  
        return false;  
    }  

    static void loadPixelsToBuffer(const uint8_t* arr, size_t size, uint8_t* dst) {  
        memcpy(dst, arr, size);  
    }  

    void loadPixels(const uint8_t *arr, size_t size){  
        loadPixelsToBuffer(arr, size, _frameHolder);  
        loadFrame(_frameHolder);  
    }  

    void loadWrapper(const uint8_t* frames, uint32_t frameCount) {  
        _currentFrame = 0;  
        _frames = frames;  
        _framesCount = frameCount;  
    }  
    // WARNING: callbacks are fired from ISR. The execution time will be limited.  
    void setCallback(voidFuncPtr callBack){  
        _callBack = callBack;  
    }  

    void clear() {  
        memset(framebuffer, 0, NUM_LEDS);  
#ifdef MATRIX_WITH_ARDUINOGRAPHICS  
        memset(_canvasBuffer, 0, sizeof(_canvasBuffer));  
#endif  
    }  

#ifdef MATRIX_WITH_ARDUINOGRAPHICS  
    virtual void set(int x, int y, uint8_t r, uint8_t g, uint8_t b) {  
      if (y >= canvasHeight || x >= canvasWidth || y < 0 || x < 0) {  
        return;  
      }  
      // 将颜色的亮度转换为 0~255 的值  
      uint8_t brightness = (uint16_t)(r + g + b) / 3;  
      _canvasBuffer[y][x] = brightness;  
    }  

    void endText(int scrollDirection = NO_SCROLL) {  
      ArduinoGraphics::endText(scrollDirection);  
      renderBitmap(_canvasBuffer, canvasHeight, canvasWidth);  
    }  

    // 显示绘制内容或在渲染动态动画时捕获  
    void endDraw() {  
      ArduinoGraphics::endDraw();  

      if (!captureAnimation) {  
        renderBitmap(_canvasBuffer, canvasHeight, canvasWidth);  
      } else {  
        if (captureAnimationHowManyRemains >= NUM_LEDS) {  
          loadPixelsToBuffer(&_canvasBuffer[0][0], sizeof(_canvasBuffer), captureAnimationFrame);  
          captureAnimationFrame += NUM_LEDS;  
          captureAnimationHowManyRemains -= NUM_LEDS;  
        }  
      }  
    }  

    void endTextToAnimationBuffer(int scrollDirection, uint8_t frames[][NUM_LEDS], uint32_t howManyMax, uint32_t& howManyUsed) {  
      captureAnimationFrame = &frames[0][0];  
      captureAnimationHowManyRemains = howManyMax * NUM_LEDS;  

      captureAnimation = true;  
      ArduinoGraphics::textScrollSpeed(0);  
      ArduinoGraphics::endText(scrollDirection);  
      ArduinoGraphics::textScrollSpeed(_textScrollSpeed);  
      captureAnimation = false;  

      howManyUsed = (howManyMax * NUM_LEDS - captureAnimationHowManyRemains) / NUM_LEDS;  
    }  

    void textScrollSpeed(unsigned long speed) {  
      ArduinoGraphics::textScrollSpeed(speed);  
      _textScrollSpeed = speed;  
    }  

  private:  
    uint8_t* captureAnimationFrame = nullptr;  
    uint32_t captureAnimationHowManyRemains = 0;  
    bool captureAnimation = false;  
    static const byte canvasWidth = 12;  
    static const byte canvasHeight = 8;  
    uint8_t _canvasBuffer[canvasHeight][canvasWidth] = {{0}};  
    unsigned long _textScrollSpeed = 100;  
#endif  

private:  
    int _currentFrame = 0;  
    uint8_t _frameHolder[NUM_LEDS];  
    const uint8_t* _frames;  
    uint32_t _framesCount;  
    uint32_t _interval = 0;  
    uint32_t _lastInterval = 0;  
    bool _loop = false;  
    FspTimer _ledTimer;  
    bool _sequenceDone = false;  
    voidFuncPtr _callBack;  

    static void turnOnLedISR(timer_callback_args_t *arg) {  
        static volatile int i_isr = 0;  
        static volatile uint8_t pwmCounter = 0;  
        pwmCounter = (pwmCounter + 1) % (PWM_MAX + 1);  

        uint8_t brightness = framebuffer[i_isr];  
        bool ledOn = brightness > pwmCounter;  

        turnLed(i_isr, ledOn);  
        i_isr = (i_isr + 1) % NUM_LEDS;  

        if (arg != nullptr && arg->p_context != nullptr) {  
            ArduinoLEDMatrix* _m = (ArduinoLEDMatrix*)arg->p_context;  
            if (_m->_interval != 0 && millis() - _m->_lastInterval > _m->_interval) {  
                _m->next();  
                _m->_lastInterval = millis();  
            }  
        }  
    }  
};  