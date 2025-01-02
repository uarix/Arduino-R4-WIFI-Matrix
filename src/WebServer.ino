#include <Arduino.h>
#include "WiFiS3.h"
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix_PWM.h"  // 可调光的 LED 矩阵库
#include "Arduino_Secrets.h"
#include <deque>

// WiFi 设置
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// Esp32 Web服务设置
WiFiServer server(80);

int status = WL_IDLE_STATUS;
unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 30000;  // 每30秒检查WiFi状态

ArduinoLEDMatrix matrix;

// LED矩阵设置
#define ROWS 8
#define COLUMNS 12
uint8_t frame[ROWS][COLUMNS] = { 0 };
int currentBrightness = -1;

// 动画相关
bool animationEnabled = false;
unsigned long lastAnimationUpdate = 0;
unsigned long animationInterval = 80;  // 默认动画更新间隔(ms)
int animationFrame = 0;

// 新增显示模式
enum DisplayMode {
  NORMAL,
  ANIMATION,
  WAVE,
  PULSE,
  BREATHING,      // 呼吸效果
  TWINKLE_STARS,  // 闪烁星星效果
  MARQUEE,        // 跑马灯效果
  RAIN,           // 雨滴效果
  SPARKLE,        // 火花效果
  SNAKE,          // 贪吃蛇效果
  SUN_ANIMATION,
  STARTUP  // 启动动画
};
DisplayMode currentMode = NORMAL;

// 贪吃蛇效果相关
struct SnakeSegment {
  int x;
  int y;
};
std::deque<SnakeSegment> snake;  // 蛇的身体部分
int snakeDirection = 0;          // 0:右 1:下 2:左 3:上

// 添加食物的位置
int foodX = 0;
int foodY = 0;


void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  matrix.begin();
  matrix.clear();  // 清空显示

  initSnake();

  // 播放启动动画
  currentMode = STARTUP;
  animationEnabled = false;
  setLight(80);  // 设置初始亮度

  randomSeed(analogRead(0));  // 初始化随机数生成器

  // 连接WiFi
  connectToWiFi();

  server.begin();
  printWifiStatus();
}

void connectToWiFi() {
  int attempts = 0;
  while (status != WL_CONNECTED && attempts < 10) {
    Serial.print("尝试连接到 SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    attempts++;
    delay(5000);
  }

  if (status != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi. Please check credentials.");
  }
}

void loop() {
  // 处理动画
  if (animationEnabled) {
    digitalWrite(LED_BUILTIN, 1);
    if (currentMode == STARTUP) {
      // 播放启动动画
      playAnimation(LEDMATRIX_ANIMATION_STARTUP, sizeof(LEDMATRIX_ANIMATION_STARTUP) / sizeof(LEDMATRIX_ANIMATION_STARTUP[0]));
    } else {
      updateAnimation();
    }
  } else {
    digitalWrite(LED_BUILTIN, 0);
  }

  // 检查WiFi连接
  if (millis() - lastWifiCheck > WIFI_CHECK_INTERVAL) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost. Reconnecting...");
      connectToWiFi();
    }
    lastWifiCheck = millis();
  }

  // 处理客户端请求
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
}

void handleClient(WiFiClient client) {
  String currentLine = "";
  boolean currentLineIsBlank = true;

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();

      if (c == '\n' && currentLineIsBlank) {
        sendResponse(client);
        break;
      }

      if (c == '\n') {
        handleRequest(currentLine);
        currentLine = "";
        currentLineIsBlank = true;
      } else if (c != '\r') {
        currentLineIsBlank = false;
        currentLine += c;
      }
    }
  }

  delay(1);
  client.stop();
}

void handleRequest(String request) {
  if (request.startsWith("GET /on")) {
    setLight(100);
    currentMode = NORMAL;
    animationEnabled = false;
  } else if (request.startsWith("GET /off")) {
    matrix.clear();
    currentMode = NORMAL;
    animationEnabled = false;
  } else if (request.startsWith("GET /brightness?value=")) {
    int valueIndex = request.indexOf('=');
    int spaceIndex = request.indexOf(' ', valueIndex);
    int brightness = request.substring(valueIndex + 1, spaceIndex).toInt();
    setLight(brightness);
  } else if (request.startsWith("GET /mode/animation")) {
    currentMode = ANIMATION;
    animationEnabled = true;
    animationFrame = 0;
    animationInterval = 80;
  } else if (request.startsWith("GET /mode/wave")) {
    currentMode = WAVE;
    animationEnabled = true;
    animationFrame = 0;
    animationInterval = 80;
  } else if (request.startsWith("GET /mode/pulse")) {
    currentMode = PULSE;
    animationEnabled = true;
    animationFrame = 0;
    animationInterval = 80;
  } else if (request.startsWith("GET /mode/breathing")) {
    currentMode = BREATHING;
    animationEnabled = true;
    animationFrame = 0;
    animationInterval = 30;
  } else if (request.startsWith("GET /mode/twinkle")) {
    currentMode = TWINKLE_STARS;
    animationEnabled = true;
    animationInterval = 150;
  } else if (request.startsWith("GET /mode/marquee")) {
    currentMode = MARQUEE;
    animationEnabled = true;
    animationFrame = 0;
    animationInterval = 100;
  } else if (request.startsWith("GET /mode/rain")) {
    currentMode = RAIN;
    animationEnabled = true;
    animationFrame = 0;
    animationInterval = 100;
  } else if (request.startsWith("GET /mode/sparkle")) {
    currentMode = SPARKLE;
    animationEnabled = true;
    animationInterval = 50;
  } else if (request.startsWith("GET /mode/snake")) {
    currentMode = SNAKE;
    animationEnabled = true;
    animationFrame = 0;
    animationInterval = 200;
    initSnake();
  } else if (request.startsWith("GET /mode/sun")) {
    currentMode = SUN_ANIMATION;
    animationEnabled = true;
    animationFrame = 0;
    animationInterval = 50;
  } else if (request.startsWith("GET /mode/startup")) {
    currentMode = STARTUP;
    animationEnabled = true;
  }
}

void sendResponse(WiFiClient client) {
  // HTTP头
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  // HTML页面
  client.println("<!DOCTYPE HTML>");
  client.println("<html lang='zh-CN'>");
  client.println("<head>");
  client.println("<meta charset='UTF-8'>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  client.println("<title>LED Matrix Control</title>");
  client.println("<style>");
  client.println("body{font-family:Arial,sans-serif;text-align:center;margin:20px;background-color:#f0f0f0;}");
  client.println(".container{max-width:800px;margin:0 auto;padding:20px;background-color:white;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1);}");
  client.println(".button{background-color:#4CAF50;border:none;color:white;padding:10px 20px;text-align:center;font-size:14px;margin:5px;cursor:pointer;border-radius:5px;transition:0.3s;}");
  client.println(".button:hover{background-color:#45a049;}");
  client.println(".slider{width:300px;margin:20px;}");
  client.println("</style>");
  client.println("</head>");
  client.println("<body>");
  client.println("<div class='container'>");
  client.println("<h1>LED Matrix Control</h1>");

  // 基本控制
  client.println("<div class='controls'>");
  client.println("<button class='button' onclick=\"fetch('/on')\">开灯</button>");
  client.println("<button class='button' onclick=\"fetch('/off')\">关灯</button>");

  // 亮度控制
  client.println("<div class='brightness'>");
  client.println("<p>亮度: <span id='brightnessValue'>50</span>%</p>");
  client.println("<input type='range' min='0' max='100' value='50' class='slider' id='brightnessSlider'>");
  client.println("</div>");

  // 模式选择
  client.println("<div class='modes'>");
  client.println("<h3>显示模式</h3>");
  client.println("<button class='button' onclick=\"fetch('/mode/animation')\">动画模式</button>");
  client.println("<button class='button' onclick=\"fetch('/mode/wave')\">波浪模式</button>");
  client.println("<button class='button' onclick=\"fetch('/mode/pulse')\">脉冲模式</button>");
  client.println("<button class='button' onclick=\"fetch('/mode/breathing')\">呼吸模式</button>");
  client.println("<button class='button' onclick=\"fetch('/mode/twinkle')\">星星闪烁</button>");
  client.println("<button class='button' onclick=\"fetch('/mode/marquee')\">跑马灯</button>");
  client.println("<button class='button' onclick=\"fetch('/mode/rain')\">雨滴效果</button>");
  client.println("<button class='button' onclick=\"fetch('/mode/sparkle')\">火花效果</button>");
  client.println("<button class='button' onclick=\"fetch('/mode/snake')\">贪吃蛇</button>");
  client.println("<button class='button' onclick=\"fetch('/mode/sun')\">太阳动画</button>");
  client.println("<button class='button' onclick=\"fetch('/mode/startup')\">启动动画</button>");
  client.println("</div>");

  // JavaScript
  client.println("<script>");
  client.println("var slider=document.getElementById('brightnessSlider');");
  client.println("var output=document.getElementById('brightnessValue');");
  client.println("slider.oninput=function(){output.innerHTML=this.value;}");
  client.println("slider.onchange=function(){fetch('/brightness?value='+this.value);}");
  client.println("</script>");

  client.println("</div></body></html>");
}

void updateAnimation() {
  if (millis() - lastAnimationUpdate >= animationInterval) {
    lastAnimationUpdate = millis();

    switch (currentMode) {
      case ANIMATION:
        updateBasicAnimation();
        break;
      case WAVE:
        updateWaveAnimation();
        break;
      case PULSE:
        updatePulseAnimation();
        break;
      case BREATHING:
        updateBreathingEffect();
        break;
      case TWINKLE_STARS:
        updateTwinkleStarsEffect();
        break;
      case MARQUEE:
        updateMarqueeEffect();
        break;
      case RAIN:
        updateRainEffect();
        break;
      case SPARKLE:
        updateSparkleEffect();
        break;
      case SUN_ANIMATION:
        updateSunAnimation();
        break;
      case SNAKE:
        updateSnakeEffect();
        break;
      default:
        break;
    }
  }
}

void playAnimation(const uint32_t animationArray[][4], size_t frameCount) {
  static size_t currentFrame = 0;
  static unsigned long lastFrameTime = 0;

  if (millis() - lastFrameTime >= animationArray[currentFrame][3]) {
    // 更新帧
    clearFrame();

    // 解析当前帧的数据
    uint32_t data[3];
    data[0] = animationArray[currentFrame][0];
    data[1] = animationArray[currentFrame][1];
    data[2] = animationArray[currentFrame][2];

    // 将数据转换为帧缓冲区
    for (int i = 0; i < ROWS; i++) {
      for (int j = 0; j < COLUMNS; j++) {
        int bitIndex = i * COLUMNS + j;
        int dataIndex = bitIndex / 32;
        int bitPosition = 31 - (bitIndex % 32);  // 高位在前

        uint8_t pixelOn = (data[dataIndex] >> bitPosition) & 0x01;
        frame[i][j] = pixelOn ? 255 : 0;
      }
    }

    // 显示帧
    matrix.renderBitmap(frame, ROWS, COLUMNS);

    // 准备下一帧
    lastFrameTime = millis();
    currentFrame++;

    if (currentFrame >= frameCount) {
      // 动画结束
      currentFrame = 0;
      animationEnabled = false;
      currentMode = NORMAL;
    }
  }
}

void updateBasicAnimation() {
  clearFrame();
  // 实现简单的动画效果
  for (int i = 0; i < ROWS; i++) {
    int col = (animationFrame + i) % COLUMNS;
    frame[i][col] = 255;  // 设置亮度为最大值
  }
  animationFrame = (animationFrame + 1) % COLUMNS;
  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void updateSunAnimation() {
  static float phase = 0.0;
  phase += 0.2;
  if (phase >= 2 * PI) {
    phase -= 2 * PI;
  }

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      // 计算与中心的距离
      int dx = i - ROWS / 2;
      int dy = j - COLUMNS / 2;
      float distance = sqrt(dx * dx + dy * dy);

      // 计算亮度波动
      float brightness = (sin(distance - phase) + 1) * 0.5 * 255;
      frame[i][j] = (uint8_t)constrain(brightness, 0, 255);
    }
  }

  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void updateTwinkleStarsEffect() {
  // 减弱所有LED的亮度，产生尾迹效果
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      frame[i][j] = frame[i][j] * 0.8;  // 调整衰减系数
    }
  }

  // 随机点亮一些LED
  for (int i = 0; i < 5; i++) {  // 每次更新5个星星，数量可调整
    int x = random(0, ROWS);
    int y = random(0, COLUMNS);
    frame[x][y] = random(150, 256);  // 随机亮度
  }

  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void updateWaveAnimation() {
  clearFrame();
  for (int i = 0; i < COLUMNS; i++) {
    int height = sin((animationFrame + i) * 0.2) * 3.5 + 4;  // 调整参数以适应矩阵大小
    frame[height % ROWS][i] = 255;                           // 设置亮度为最大值
  }
  animationFrame = (animationFrame + 1) % 360;
  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void updateBreathingEffect() {
  static float brightness = 0;
  static int direction = 1;

  brightness += direction * 0.02;
  if (brightness >= 1.0) {
    brightness = 1.0;
    direction = -1;
  } else if (brightness <= 0.0) {
    brightness = 0.0;
    direction = 1;
  }

  uint8_t led_val = brightness * 255;
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      frame[i][j] = led_val;
    }
  }
  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void updateMarqueeEffect() {
  clearFrame();

  for (int i = 0; i < ROWS; i++) {
    frame[i][animationFrame % COLUMNS] = 255;
  }

  animationFrame = (animationFrame + 1) % COLUMNS;
  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void updatePulseAnimation() {
  clearFrame();
  for (int i = 0; i < COLUMNS; i++) {
    float brightness = (sin((animationFrame + i) * 0.3) + 1) * 0.5;  // 计算每列的亮度
    uint8_t led_val = brightness * 255;
    for (int j = 0; j < ROWS; j++) {
      frame[j][i] = led_val;
    }
  }
  animationFrame = (animationFrame + 1) % 360;
  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void updateRainEffect() {
  // 下移所有LED，模拟雨滴下落
  for (int i = ROWS - 1; i > 0; i--) {
    for (int j = 0; j < COLUMNS; j++) {
      frame[i][j] = frame[i - 1][j];
    }
  }

  // 随机在顶部生成新的雨滴
  for (int j = 0; j < COLUMNS; j++) {
    frame[0][j] = (random(0, 10) > 7) ? 255 : 0;
  }

  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void updateSparkleEffect() {
  // 随机闪烁的火花效果
  clearFrame();
  int sparks = random(5, 15);  // 每次生成的火花数量
  for (int i = 0; i < sparks; i++) {
    int x = random(0, ROWS);
    int y = random(0, COLUMNS);
    frame[x][y] = 255;
  }
  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void initSnake() {
  snake.clear();
  snake.push_back({ ROWS / 2, COLUMNS / 2 });
  snakeDirection = random(0, 4);  // 随机方向

  // 生成食物
  generateFood();
}

void generateFood() {
  bool validPosition = false;
  while (!validPosition) {
    foodX = random(0, ROWS);
    foodY = random(0, COLUMNS);

    // 确保食物不在蛇的身体上
    validPosition = true;
    for (const auto& segment : snake) {
      if (segment.x == foodX && segment.y == foodY) {
        validPosition = false;
        break;
      }
    }
  }
}
void updateSnakeEffect() {
  // 获取蛇头的位置
  SnakeSegment head = snake.front();
  SnakeSegment newHead = head;

  // 计算蛇头与食物的位置差
  int deltaX = foodX - head.x;
  int deltaY = foodY - head.y;

  // 根据食物的位置调整蛇的移动方向
  if (abs(deltaX) > abs(deltaY)) {
    if (deltaX > 0) {
      snakeDirection = 1;  // 下
    } else if (deltaX < 0) {
      snakeDirection = 3;  // 上
    }
  } else {
    if (deltaY > 0) {
      snakeDirection = 0;  // 右
    } else if (deltaY < 0) {
      snakeDirection = 2;  // 左
    }
  }

  // 根据方向移动蛇头
  switch (snakeDirection) {
    case 0:  // 右
      newHead.y = (head.y + 1) % COLUMNS;
      break;
    case 1:  // 下
      newHead.x = (head.x + 1) % ROWS;
      break;
    case 2:  // 左
      newHead.y = (head.y - 1 + COLUMNS) % COLUMNS;
      break;
    case 3:  // 上
      newHead.x = (head.x - 1 + ROWS) % ROWS;
      break;
  }

  // 检查是否撞到自身
  for (const auto& segment : snake) {
    if (segment.x == newHead.x && segment.y == newHead.y) {
      // 撞到了自身，重新开始游戏
      initSnake();
      return;
    }
  }

  // 将新头部加入蛇的身体
  snake.push_front(newHead);

  // 检查是否吃到食物
  if (newHead.x == foodX && newHead.y == foodY) {
    // 吃到了食物，生成新的食物
    generateFood();
  } else {
    // 未吃到食物，移除尾部（保持长度不变）
    snake.pop_back();
  }

  // 绘制蛇和食物
  clearFrame();

  // 绘制食物
  frame[foodX][foodY] = 255;

  // 绘制蛇
  uint8_t brightness = 255;
  uint8_t decrement = 255 / snake.size();

  for (const auto& segment : snake) {
    frame[segment.x][segment.y] = brightness;
    if (brightness > decrement) {
      brightness -= decrement;
    } else {
      brightness = 0;
    }
  }

  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void setLight(int brightness) {
  if (brightness == currentBrightness) {
    Serial.println("[灯光]亮度未发生更新: " + String(brightness));
    return;  // 跳过冗余的更新
  }
  clearFrame();
  currentBrightness = brightness;
  Serial.println("[灯光]设置LED亮度: " + String(brightness));

  if (brightness <= 0) {
    // 全部熄灭
    Serial.println("[灯光]关闭全部LED");
    matrix.clear();
  } else if (brightness >= 90) {
    setSunIcon();
  } else {
    setBrightnessLevel(brightness);
  }

  matrix.renderBitmap(frame, ROWS, COLUMNS);
}

void clearFrame() {
  memset(frame, 0, sizeof(frame));
}

void setSunIcon() {
  clearFrame();
  uint8_t sunIcon[ROWS][COLUMNS] = {
    { 0, 0, 60, 90, 120, 150, 150, 120, 90, 60, 0, 0 },
    { 0, 60, 90, 120, 180, 220, 220, 180, 120, 90, 60, 0 },
    { 60, 90, 150, 200, 230, 255, 255, 230, 200, 150, 90, 60 },
    { 90, 150, 200, 230, 255, 255, 255, 255, 230, 200, 150, 90 },
    { 90, 150, 200, 230, 255, 255, 255, 255, 230, 200, 150, 90 },
    { 60, 90, 150, 200, 230, 255, 255, 230, 200, 150, 90, 60 },
    { 0, 60, 90, 120, 180, 220, 220, 180, 120, 90, 60, 0 },
    { 0, 0, 60, 90, 120, 150, 150, 120, 90, 60, 0, 0 },
  };
  memcpy(frame, sunIcon, sizeof(frame));
}

void setBrightnessLevel(int brightness) {
  //亮度值从 1-89 映射到 1-254
  //uint8_t led_val = map(brightness, 1, 89, 1, 254);
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      frame[i][j] = brightness;
    }
  }
}

void printWifiStatus() {
  Serial.print("已连接到SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP 地址: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("信号强度 (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.print("要控制 LED, 请访问 http://");
  Serial.println(ip);

  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(60);
  String text = " IP: " + ip.toString() + " ";  
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText(SCROLL_LEFT);
  matrix.endDraw();

  animationEnabled = true;
}