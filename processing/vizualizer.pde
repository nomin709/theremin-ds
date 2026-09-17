// ============================================================
// Processing Visualization for ESP32 Theremin
// ============================================================

import processing.serial.*;

Serial myPort;

float distVal = -1;
float pitchVal = 0;
float vrawVal = 0;
float vnormVal = 0;
float volVal = 0;

int calOkVal = 0;
int playVal = 0;
int powerVal = 1;
int useDefVal = 0;

boolean paused = false;
boolean showCombinedGraph = true;
boolean calibrationActive = false;

String systemMessage = "SYSTEM ON";
int systemMessageTimer = 0;

// Graph history
int HISTORY_LEN = 300;
float[] pitchHistory = new float[HISTORY_LEN];
float[] volHistory = new float[HISTORY_LEN];

// Display ranges
float minPitch = 250;
float maxPitch = 700;

void setup() {
  pixelDensity(1);
  size(1200, 750);
  surface.setTitle("ESP32 Theremin Visualization");

  printArray(Serial.list());

  // Change port index if needed
  myPort = new Serial(this, Serial.list()[0], 115200);
  myPort.bufferUntil('\n');

  resetHistory();
  textFont(createFont("Arial", 18));
}

void draw() {
  float layoutY = 120;
  float barY = layoutY;
  float barH = 255;
  float graphX = 390;
  float graphW = 760;

  drawBackground();

  if (systemMessageTimer > 0) {
    systemMessageTimer--;
  }

  if (calibrationActive) {
    fill(255, 70);
    noStroke();
    rect(0, 0, width, height);
  }

  fill(255);
  textAlign(CENTER, TOP);
  textSize(30);
  text("ESP32 Theremin", width/2, 18);

  drawPitchBar(70, barY, 110, barH);
  drawVolumeBar(220, barY, 110, barH);

  if (showCombinedGraph) {
    drawCombinedGraph(graphX, layoutY, graphW, 255);
  } else {
    drawSeparateGraphs(graphX, layoutY, graphW, 410);
  }

  drawInfoSection(70, 450);

  if (systemMessageTimer > 0) {
    drawSystemMessage();
  }

  if (calibrationActive) {
    drawCalibrationBanner();
  }
}

void drawInfoSection(float x, float y) {
  fill(255);
  textAlign(LEFT, TOP);
  textSize(18);

  float lineGap = 30;

  text("Power: " + (powerVal == 1 ? "ON" : "OFF"), x, y);
  text("Calibrated: " + (calOkVal == 1 ? "YES" : "NO"), x, y + lineGap);
  text("Playing: " + (playVal == 1 ? "YES" : "NO"), x, y + 2 * lineGap);
  text("Distance: " + (distVal < 0 ? "No Hand" : nf(distVal, 0, 1) + " cm"), x, y + 3 * lineGap);
  text("Raw Light: " + nf(vrawVal, 0, 0), x, y + 4 * lineGap);
  text("Norm Light: " + nf(vnormVal, 0, 2), x, y + 5 * lineGap);

  textSize(16);
  textAlign(LEFT, TOP);
  text("Keys: r = reset, p = pause, m = toggle graph", x + 770, height - 40);
}

void serialEvent(Serial p) {
  if (paused) return;

  String line = p.readStringUntil('\n');
  if (line == null) return;
  
  line = trim(line);
  if (line.length() == 0 || line.startsWith("SYSTEM") || line.startsWith("CALIBRATION")) return;

  String[] parts = split(line, ',');
  if (parts.length != 8) return; // Expecting our 8-column CSV payload

  try {
    // Map incoming CSV columns to visualizer variables
    float rawDist    = float(parts[0]);
    distVal          = float(parts[1]); // smooth_dist
    float velocity   = float(parts[2]);
    float accel      = float(parts[3]);
    float rawPhoto   = float(parts[4]);
    vnormVal         = float(parts[5]); // smooth_photo (0.0 to 1.0)
    pitchVal         = float(parts[6]); // note frequency in Hz
    int mode         = int(parts[7]);

    // Map light to volume bar for visualization
    volVal = vnormVal; 
    
    // Set active states
    powerVal = 1;
    playVal = (distVal > 0 ? 1 : 0);
    calOkVal = 1;

    updateHistory();
  } catch (Exception e) {
    // Ignore malformed startup lines
  }
}

float parseSafeFloat(String s) {
  try {
    return Float.parseFloat(s);
  } catch(Exception e) {
    return 0;
  }
}

void updateHistory() {
  for (int i = 0; i < HISTORY_LEN - 1; i++) {
    pitchHistory[i] = pitchHistory[i + 1];
    volHistory[i] = volHistory[i + 1];
  }

  pitchHistory[HISTORY_LEN - 1] = pitchVal;
  volHistory[HISTORY_LEN - 1] = volVal;
}

void resetHistory() {
  for (int i = 0; i < HISTORY_LEN; i++) {
    pitchHistory[i] = 0;
    volHistory[i] = 0;
  }
}

void drawBackground() {
  if (powerVal == 0) {
    background(20);
    return;
  }

  background(25, 35, 60);

  if (useDefVal == 1) {
    fill(255, 18);
    noStroke();
    rect(0, 0, width, height);
  }
}

void drawPitchBar(float x, float y, float w, float h) {
  stroke(255);
  noFill();
  rect(x, y, w, h);

  float mapped = map(constrain(pitchVal, minPitch, maxPitch), minPitch, maxPitch, 0, h);

  fill(255, 210, 90);
  noStroke();
  rect(x, y + h - mapped, w, mapped);

  fill(255);
  textAlign(CENTER, BOTTOM);
  textSize(22);
  text("Pitch", x + w/2, y - 12);

  textAlign(CENTER, TOP);
  textSize(18);
  text(nf(pitchVal, 0, 0) + " Hz", x + w/2, y + h + 10);
}

void drawVolumeBar(float x, float y, float w, float h) {
  stroke(255);
  noFill();
  rect(x, y, w, h);

  float mapped = map(constrain(volVal, 0, 1), 0, 1, 0, h);

  fill(100, 220, 255);
  noStroke();
  rect(x, y + h - mapped, w, mapped);

  if (useDefVal == 1) {
    fill(255, 220, 80);
    textAlign(CENTER, TOP);
    textSize(15);
    text("DEFAULT", x + w/2, y + 10);
  }

  fill(255);
  textAlign(CENTER, BOTTOM);
  textSize(22);
  text("Volume", x + w/2, y - 12);

  textAlign(CENTER, TOP);
  textSize(18);
  text(nf(volVal * 100, 0, 1) + "%", x + w/2, y + h + 10);
}

void drawCombinedGraph(float x, float y, float w, float h) {
  fill(255);
  textAlign(CENTER, BOTTOM);
  textSize(22);
  text("Scrolling Graph: Pitch + Volume", x + w/2, y - 10);

  stroke(255);
  noFill();
  rect(x, y, w, h);

  float topPad = 18;
  float bottomPad = 12;

  stroke(255, 220, 100);
  noFill();
  beginShape();
  for (int i = 0; i < HISTORY_LEN; i++) {
    float px = map(i, 0, HISTORY_LEN - 1, x + 8, x + w - 8);
    float py = map(constrain(pitchHistory[i], minPitch, maxPitch),
      minPitch, maxPitch, y + h - bottomPad, y + topPad);
    vertex(px, py);
  }
  endShape();

  stroke(100, 220, 255);
  noFill();
  beginShape();
  for (int i = 0; i < HISTORY_LEN; i++) {
    float px = map(i, 0, HISTORY_LEN - 1, x + 8, x + w - 8);
    float py = map(constrain(volHistory[i], 0, 1),
      0, 1, y + h - bottomPad, y + topPad);
    vertex(px, py);
  }
  endShape();

  float legendY = y + 18;

  fill(255, 220, 100);
  noStroke();
  ellipse(x + 30, legendY, 10, 10);
  fill(255);
  textAlign(LEFT, CENTER);
  textSize(16);
  text("Pitch", x + 45, legendY);

  fill(100, 220, 255);
  ellipse(x + 130, legendY, 10, 10);
  fill(255);
  text("Volume", x + 145, legendY);
}

void drawSeparateGraphs(float x, float y, float w, float h) {
  float titleGap = 28;
  float panelGap = 34;
  float hEach = (h - panelGap - 2 * titleGap) / 2.0;

  float y1Box = y + titleGap;

  fill(255);
  textAlign(CENTER, BOTTOM);
  textSize(22);
  text("Pitch Graph", x + w/2, y1Box - 8);

  stroke(255);
  noFill();
  rect(x, y1Box, w, hEach);

  stroke(255, 220, 100);
  noFill();
  beginShape();
  for (int i = 0; i < HISTORY_LEN; i++) {
    float px = map(i, 0, HISTORY_LEN - 1, x + 8, x + w - 8);
    float py = map(constrain(pitchHistory[i], minPitch, maxPitch),
      minPitch, maxPitch, y1Box + hEach - 10, y1Box + 10);
    vertex(px, py);
  }
  endShape();

  float y2Title = y1Box + hEach + panelGap;
  float y2Box = y2Title + titleGap;

  fill(255);
  textAlign(CENTER, BOTTOM);
  textSize(22);
  text("Volume Graph", x + w/2, y2Box - 8);

  stroke(255);
  noFill();
  rect(x, y2Box, w, hEach);

  stroke(100, 220, 255);
  noFill();
  beginShape();
  for (int i = 0; i < HISTORY_LEN; i++) {
    float px = map(i, 0, HISTORY_LEN - 1, x + 8, x + w - 8);
    float py = map(constrain(volHistory[i], 0, 1),
      0, 1, y2Box + hEach - 10, y2Box + 10);
    vertex(px, py);
  }
  endShape();
}

void drawSystemMessage() {
  fill(0, 180);
  noStroke();
  rect(width/2 - 170, 22, 340, 60, 12);

  fill(255);
  textAlign(CENTER, CENTER);
  textSize(24);
  text(systemMessage, width/2, 52);
}

void drawCalibrationBanner() {
  fill(255, 240);
  noStroke();
  rect(width/2 - 190, height - 90, 380, 46, 12);

  fill(20);
  textAlign(CENTER, CENTER);
  textSize(22);
  text("CALIBRATION ACTIVE", width/2, height - 67);
}

void keyPressed() {
  if (key == 'r' || key == 'R') {
    resetHistory();
  } else if (key == 'p' || key == 'P') {
    paused = !paused;
  } else if (key == 'm' || key == 'M') {
    showCombinedGraph = !showCombinedGraph;
  }
}
