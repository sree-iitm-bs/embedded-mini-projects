#include <WiFi.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputInternalDAC.h>

const char* ssid = "Shakthi";
const char* password = "15120909";
const String apiKey = "02606c769a4149acb63b886b334f4ed0"; // Your VoiceRSS API key

AudioGeneratorWAV *wav = nullptr;
AudioFileSourceHTTPStream *file = nullptr;
AudioOutputInternalDAC *out = nullptr;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.println("Enter text to speak:");
}

void loop() {
  if (Serial.available()) {
    String text = Serial.readStringUntil('\n');
    text.trim();

    if (text.length() > 0) {
      cleanupAudio();

      String encoded = urlencode(text);
      String url = "http://api.voicerss.org/?key=" + apiKey +
                   "&hl=en-us&src=" + encoded + "&c=WAV&f=8khz_8bit_mono";

      file = new AudioFileSourceHTTPStream(url.c_str());
      out = new AudioOutputInternalDAC();
      out->SetGain(1.0);

      wav = new AudioGeneratorWAV();
      if (!wav->begin(file, out)) {
        Serial.println("Failed to start audio playback!");
        cleanupAudio();
      }
    }
  }

  if (wav && wav->isRunning()) {
    wav->loop();
  } else if (wav && !wav->isRunning()) {
    cleanupAudio();
  }
}

void cleanupAudio() {
  if (wav) {
    wav->stop();
    delete wav;
    wav = nullptr;
  }

  if (file) {
    delete file;
    file = nullptr;
  }

  if (out) {
    delete out;
    out = nullptr;
  }

  // Reduce noise: reset DAC output
  dacWrite(25, 0);  // GPIO25
  dacWrite(26, 0);  // GPIO26 (optional, even though we use 25)
}

String urlencode(String str) {
  String encoded = "";
  char c;
  char code0, code1;

  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encoded += '%';
      encoded += char(code0 < 10 ? code0 + '0' : code0 - 10 + 'A');
      encoded += char(code1 < 10 ? code1 + '0' : code1 - 10 + 'A');
    }
  }
  return encoded;
}
