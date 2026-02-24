#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// ========== CONFIGURAÇÕES DE REDE ==========
const char* ssid = "nome da rede";
const char* password = "senha da rede";

// ========== LIMITES DE PULSO PARA MG996R ==========
#define SERVO_MIN_US 500   // ajuste p/ 600 se bater no fim
#define SERVO_MAX_US 2500  // ajuste p/ 2400 se bater no fim

// ========== CONFIGURAÇÃO DOS SERVOS ==========
Servo servo1;
Servo servo2;
Servo servo3;

// Mantidos seus pinos originais:
const int SERVO1_PIN = D1; // GPIO5
const int SERVO2_PIN = D2; // GPIO4
const int SERVO3_PIN = D3; // GPIO0 (pino de boot; se funcionar, ok)

// ========== CONFIGURAÇÃO DO ELETROÍMÃ ==========
const int ELETROIMÃ_PIN = D0; // GPIO16
bool eletroima_ligado = false;

int posicao1 = 90;
int posicao2 = 90;
int posicao3 = 90;

ESP8266WebServer server(80);

// ========== FUNÇÃO PARA MOVER SERVO COM LIMITES ==========
void moverServo(int servoNum, int incremento) {
  int* posicaoAtual;
  Servo* servo;
  
  switch(servoNum) {
    case 1: posicaoAtual = &posicao1; servo = &servo1; break;
    case 2: posicaoAtual = &posicao2; servo = &servo2; break;
    case 3: posicaoAtual = &posicao3; servo = &servo3; break;
    default: return;
  }
  
  *posicaoAtual += incremento;
  if (*posicaoAtual > 180) *posicaoAtual = 180;
  if (*posicaoAtual < 0)   *posicaoAtual = 0;
  
  servo->write(*posicaoAtual);
}

// ========== FUNÇÃO PARA DEFINIR POSIÇÃO ABSOLUTA ==========
void definirPosicao(int servoNum, int posicao) {
  Servo* servo;
  
  if (posicao > 180) posicao = 180;
  if (posicao < 0)   posicao = 0;
  
  switch(servoNum) {
    case 1: posicao1 = posicao; servo = &servo1; break;
    case 2: posicao2 = posicao; servo = &servo2; break;
    case 3: posicao3 = posicao; servo = &servo3; break;
    default: return;
  }
  servo->write(posicao);
}

// ========== PÁGINA WEB HTML ==========
String getPaginaHTML() {
  String html = R"=====(
<!DOCTYPE html>
<html lang='pt-BR'>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>TechnoArm Manual Control</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Arial Black','Arial Bold',sans-serif;
      background: linear-gradient(135deg, #1a1a1a 0%, #2d2d2d 100%);
      min-height: 100vh; display: flex; justify-content: center; align-items: center; padding: 20px;
    }
    .container {
      background: rgba(0,0,0,0.8); border: 4px solid #e63946; border-radius: 20px;
      padding: 40px; max-width: 700px; width: 100%; box-shadow: 0 20px 60px rgba(230,57,70,0.3);
    }
    .logo { text-align: center; margin-bottom: 40px; }
    .logo h1 {
      font-size: 3em; color: #fff; text-transform: uppercase; letter-spacing: 3px;
      text-shadow: 3px 3px 0 #4361ee, 6px 6px 0 #e63946, 9px 9px 20px rgba(230,57,70,0.5);
      margin-bottom: 10px;
    }
    .logo p { color: #4361ee; font-size: 1em; font-weight: bold; letter-spacing: 2px; }
    .servo-control {
      background: linear-gradient(135deg, #2d2d2d 0%, #1a1a1a 100%);
      border: 3px solid #4361ee; border-radius: 15px; padding: 25px; margin-bottom: 20px;
    }
    .servo-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }
    .servo-title { color: #fff; font-size: 1.3em; text-transform: uppercase; letter-spacing: 1px; }
    .servo-value {
      background: linear-gradient(135deg, #e63946 0%, #c42a36 100%);
      color: #fff; padding: 8px 20px; border-radius: 8px; font-size: 1.5em; font-weight: bold;
      min-width: 80px; text-align: center; border: 2px solid #4361ee;
    }
    .servo-buttons { display: flex; gap: 15px; justify-content: center; margin-bottom: 20px; }
    .btn {
      padding: 20px 40px; font-size: 2em; font-weight: bold; border: 3px solid #4361ee; border-radius: 12px;
      cursor: pointer; color: #fff; background: linear-gradient(135deg, #2d2d2d 0%, #1a1a1a 100%);
      user-select: none; transition: all 0.2s ease; flex: 1; max-width: 150px;
    }
    .btn:hover { border-color: #e63946; box-shadow: 0 0 20px rgba(230,57,70,0.6); transform: scale(1.05); }
    .btn:active { transform: scale(0.95); background: linear-gradient(135deg, #e63946 0%, #c42a36 100%); }
    .slider-container { display: flex; align-items: center; gap: 15px; margin-top: 15px; }
    .slider-label { color: #4361ee; font-size: 0.9em; font-weight: bold; min-width: 40px; }
    .slider {
      flex: 1; -webkit-appearance: none; appearance: none; height: 12px; border-radius: 6px;
      background: linear-gradient(90deg, #1a1a1a 0%, #4361ee 50%, #e63946 100%);
      outline: none; border: 2px solid #4361ee;
    }
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none; appearance: none; width: 28px; height: 28px; border-radius: 50%;
      background: linear-gradient(135deg, #e63946 0%, #c42a36 100%);
      cursor: pointer; border: 3px solid #fff; box-shadow: 0 0 10px rgba(230,57,70,0.8); transition: all 0.2s ease;
    }
    .slider::-webkit-slider-thumb:hover { transform: scale(1.2); box-shadow: 0 0 20px rgba(230,57,70,1); }
    .slider::-moz-range-thumb {
      width: 28px; height: 28px; border-radius: 50%;
      background: linear-gradient(135deg, #e63946 0%, #c42a36 100%);
      cursor: pointer; border: 3px solid #fff; box-shadow: 0 0 10px rgba(230,57,70,0.8); transition: all 0.2s ease;
    }
    .slider::-moz-range-thumb:hover { transform: scale(1.2); box-shadow: 0 0 20px rgba(230,57,70,1); }
    
    /* ========== ESTILOS DO ELETROÍMÃ ========== */
    .magnet-control {
      background: linear-gradient(135deg, #2d2d2d 0%, #1a1a1a 100%);
      border: 3px solid #ffd60a; border-radius: 15px; padding: 25px; margin-bottom: 20px;
    }
    .magnet-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }
    .magnet-title { color: #fff; font-size: 1.3em; text-transform: uppercase; letter-spacing: 1px; }
    .magnet-status {
      color: #fff; padding: 8px 20px; border-radius: 8px; font-size: 1.2em; font-weight: bold;
      min-width: 120px; text-align: center; border: 2px solid #ffd60a;
      transition: all 0.3s ease;
    }
    .magnet-status.off {
      background: linear-gradient(135deg, #6c757d 0%, #495057 100%);
    }
    .magnet-status.on {
      background: linear-gradient(135deg, #ffd60a 0%, #ffc107 100%);
      color: #000;
      box-shadow: 0 0 20px rgba(255, 214, 10, 0.6);
      animation: pulse 1.5s infinite;
    }
    @keyframes pulse {
      0%, 100% { box-shadow: 0 0 20px rgba(255, 214, 10, 0.6); }
      50% { box-shadow: 0 0 30px rgba(255, 214, 10, 1); }
    }
    .btn-magnet {
      width: 100%; padding: 25px; font-size: 1.5em; font-weight: bold;
      border: 3px solid #ffd60a; border-radius: 12px; cursor: pointer; color: #fff;
      user-select: none; transition: all 0.3s ease;
    }
    .btn-magnet.off {
      background: linear-gradient(135deg, #6c757d 0%, #495057 100%);
    }
    .btn-magnet.off:hover {
      background: linear-gradient(135deg, #ffd60a 0%, #ffc107 100%);
      color: #000;
      box-shadow: 0 0 30px rgba(255, 214, 10, 0.8);
      transform: scale(1.02);
    }
    .btn-magnet.on {
      background: linear-gradient(135deg, #ffd60a 0%, #ffc107 100%);
      color: #000;
      box-shadow: 0 0 20px rgba(255, 214, 10, 0.6);
    }
    .btn-magnet.on:hover {
      background: linear-gradient(135deg, #6c757d 0%, #495057 100%);
      color: #fff;
      box-shadow: none;
      transform: scale(1.02);
    }
    
    .btn-reset { 
      width: 100%; background: linear-gradient(135deg, #e63946 0%, #c42a36 100%); 
      border-color: #fff; font-size: 1.3em; padding: 20px; margin-top: 10px; 
    }
    .btn-reset:hover { 
      background: linear-gradient(135deg, #ff4757 0%, #e63946 100%); 
      box-shadow: 0 0 30px rgba(230,57,70,0.8); 
    }
    @media (max-width: 600px) { 
      .logo h1 { font-size: 2em; } 
      .btn { font-size: 1.5em; padding: 15px 30px; } 
    }
  </style>
</head>
<body>
  <div class='container'>
    <div class='logo'>
      <h1>TECHNOARM</h1>
      <p>MANUAL CONTROL SYSTEM</p>
    </div>
    
    <!-- Servo 1 -->
    <div class='servo-control'>
      <div class='servo-header'>
        <span class='servo-title'>Servo 1 - Base</span>
        <span class='servo-value' id='value1'>90°</span>
      </div>
      <div class='servo-buttons'>
        <button class='btn' 
                onmousedown='iniciarMovimento(1, -5)' 
                onmouseup='pararMovimento()' 
                onmouseleave='pararMovimento()'
                ontouchstart='iniciarMovimento(1, -5)' 
                ontouchend='pararMovimento()'>−</button>
        <button class='btn' 
                onmousedown='iniciarMovimento(1, 5)' 
                onmouseup='pararMovimento()' 
                onmouseleave='pararMovimento()'
                ontouchstart='iniciarMovimento(1, 5)' 
                ontouchend='pararMovimento()'>+</button>
      </div>
      <div class='slider-container'>
        <span class='slider-label'>0°</span>
        <input type='range' min='0' max='180' value='90' class='slider' id='slider1' 
               oninput='moverPorSlider(1, this.value)'>
        <span class='slider-label'>180°</span>
      </div>
    </div>
    
    <!-- Servo 2 -->
    <div class='servo-control'>
      <div class='servo-header'>
        <span class='servo-title'>Servo 2 - Articulação</span>
        <span class='servo-value' id='value2'>90°</span>
      </div>
      <div class='servo-buttons'>
        <button class='btn' 
                onmousedown='iniciarMovimento(2, -5)' 
                onmouseup='pararMovimento()' 
                onmouseleave='pararMovimento()'
                ontouchstart='iniciarMovimento(2, -5)' 
                ontouchend='pararMovimento()'>−</button>
        <button class='btn' 
                onmousedown='iniciarMovimento(2, 5)' 
                onmouseup='pararMovimento()' 
                onmouseleave='pararMovimento()'
                ontouchstart='iniciarMovimento(2, 5)' 
                ontouchend='pararMovimento()'>+</button>
      </div>
      <div class='slider-container'>
        <span class='slider-label'>0°</span>
        <input type='range' min='0' max='180' value='90' class='slider' id='slider2' 
               oninput='moverPorSlider(2, this.value)'>
        <span class='slider-label'>180°</span>
      </div>
    </div>
    
    <!-- Servo 3 -->
    <div class='servo-control'>
      <div class='servo-header'>
        <span class='servo-title'>Servo 3 - Garra</span>
        <span class='servo-value' id='value3'>90°</span>
      </div>
      <div class='servo-buttons'>
        <button class='btn' 
                onmousedown='iniciarMovimento(3, -5)' 
                onmouseup='pararMovimento()' 
                onmouseleave='pararMovimento()'
                ontouchstart='iniciarMovimento(3, -5)' 
                ontouchend='pararMovimento()'>−</button>
        <button class='btn' 
                onmousedown='iniciarMovimento(3, 5)' 
                onmouseup='pararMovimento()' 
                onmouseleave='pararMovimento()'
                ontouchstart='iniciarMovimento(3, 5)' 
                ontouchend='pararMovimento()'>+</button>
      </div>
      <div class='slider-container'>
        <span class='slider-label'>0°</span>
        <input type='range' min='0' max='180' value='90' class='slider' id='slider3' 
               oninput='moverPorSlider(3, this.value)'>
        <span class='slider-label'>180°</span>
      </div>
    </div>
    
    <!-- ELETROÍMÃ -->
    <div class='magnet-control'>
      <div class='magnet-header'>
        <span class='magnet-title'>🧲 Eletroímã 12V/8KG</span>
        <span class='magnet-status off' id='magnet-status'>DESLIGADO</span>
      </div>
      <button class='btn-magnet off' id='btn-magnet' onclick='toggleEletroima()'>
        🔌 LIGAR ELETROÍMÃ
      </button>
    </div>
    
    <button class='btn btn-reset' onclick='resetarPosicoes()'>
      🔄 RESETAR POSIÇÕES
    </button>
  </div>
  
  <script>
    let intervalo = null;
    let servoAtual = 0;
    let incrementoAtual = 0;
    let eletroima_ligado = false;
    
    function iniciarMovimento(servo, incremento) {
      pararMovimento();
      servoAtual = servo;
      incrementoAtual = incremento;
      moverServo();
      intervalo = setInterval(moverServo, 100);
    }
    
    function pararMovimento() {
      if (intervalo) { clearInterval(intervalo); intervalo = null; }
      servoAtual = 0; incrementoAtual = 0;
    }
    
    function moverServo() {
      fetch('/mover?servo=' + servoAtual + '&inc=' + incrementoAtual)
        .then(response => response.json())
        .then(data => {
          document.getElementById('value' + data.servo).textContent = data.posicao + '°';
          document.getElementById('slider' + data.servo).value = data.posicao;
        })
        .catch(error => {
          console.error('Erro:', error);
          pararMovimento();
        });
    }
    
    function moverPorSlider(servo, posicao) {
      pararMovimento();
      fetch('/posicao?servo=' + servo + '&pos=' + posicao)
        .then(response => response.json())
        .then(data => {
          document.getElementById('value' + data.servo).textContent = data.posicao + '°';
        })
        .catch(error => { console.error('Erro:', error); });
    }
    
    function toggleEletroima() {
      fetch('/eletroima?estado=' + (eletroima_ligado ? '0' : '1'))
        .then(response => response.json())
        .then(data => {
          eletroima_ligado = data.ligado;
          atualizarInterfaceEletroima();
        })
        .catch(error => { console.error('Erro:', error); });
    }
    
    function atualizarInterfaceEletroima() {
      const statusEl = document.getElementById('magnet-status');
      const btnEl = document.getElementById('btn-magnet');
      
      if (eletroima_ligado) {
        statusEl.textContent = 'LIGADO ⚡';
        statusEl.className = 'magnet-status on';
        btnEl.textContent = '🔴 DESLIGAR ELETROÍMÃ';
        btnEl.className = 'btn-magnet on';
      } else {
        statusEl.textContent = 'DESLIGADO';
        statusEl.className = 'magnet-status off';
        btnEl.textContent = '🔌 LIGAR ELETROÍMÃ';
        btnEl.className = 'btn-magnet off';
      }
    }
    
    function resetarPosicoes() {
      pararMovimento();
      fetch('/reset')
        .then(response => response.json())
        .then(_ => {
          document.getElementById('value1').textContent = '90°';
          document.getElementById('value2').textContent = '90°';
          document.getElementById('value3').textContent = '90°';
          document.getElementById('slider1').value = 90;
          document.getElementById('slider2').value = 90;
          document.getElementById('slider3').value = 90;
        })
        .catch(error => { console.error('Erro:', error); });
    }
    
    // Carregar estado inicial do eletroímã
    window.addEventListener('load', function() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          eletroima_ligado = data.eletroima_ligado;
          atualizarInterfaceEletroima();
        })
        .catch(error => { console.error('Erro ao carregar status:', error); });
    });
    
    document.addEventListener('selectstart', e => e.preventDefault());
    document.addEventListener('touchstart', e => { if (e.touches.length > 1) e.preventDefault(); }, { passive: false });
    let lastTouchEnd = 0;
    document.addEventListener('touchend', e => {
      const now = Date.now();
      if (now - lastTouchEnd <= 300) e.preventDefault();
      lastTouchEnd = now;
    }, false);
    document.addEventListener('mouseup', pararMovimento);
    document.addEventListener('touchend', pararMovimento);
    document.addEventListener('touchcancel', pararMovimento);
  </script>
</body>
</html>
)=====";
  return html;
}

// ========== HANDLERS DE REQUISIÇÃO ==========
void handleRoot() {
  server.send(200, "text/html", getPaginaHTML());
}

void handleMover() {
  if (server.hasArg("servo") && server.hasArg("inc")) {
    int servo = server.arg("servo").toInt();
    int incremento = server.arg("inc").toInt();
    
    moverServo(servo, incremento);
    
    String json = "{";
    json += "\"servo\":" + String(servo) + ",";
    switch(servo) {
      case 1: json += "\"posicao\":" + String(posicao1); break;
      case 2: json += "\"posicao\":" + String(posicao2); break;
      case 3: json += "\"posicao\":" + String(posicao3); break;
    }
    json += "}";
    server.send(200, "application/json", json);
  } else {
    server.send(400, "text/plain", "Parâmetros ausentes");
  }
}

void handlePosicao() {
  if (server.hasArg("servo") && server.hasArg("pos")) {
    int servo = server.arg("servo").toInt();
    int posicao = server.arg("pos").toInt();
    
    definirPosicao(servo, posicao);
    
    String json = "{";
    json += "\"servo\":" + String(servo) + ",";
    switch(servo) {
      case 1: json += "\"posicao\":" + String(posicao1); break;
      case 2: json += "\"posicao\":" + String(posicao2); break;
      case 3: json += "\"posicao\":" + String(posicao3); break;
    }
    json += "}";
    server.send(200, "application/json", json);
  } else {
    server.send(400, "text/plain", "Parâmetros ausentes");
  }
}

void handleEletroima() {
  if (server.hasArg("estado")) {
    int estado = server.arg("estado").toInt();
    eletroima_ligado = (estado == 1);
    
    digitalWrite(ELETROIMÃ_PIN, eletroima_ligado ? HIGH : LOW);
    
    Serial.print("Eletroímã ");
    Serial.println(eletroima_ligado ? "LIGADO" : "DESLIGADO");
    
    String json = "{\"ligado\":";
    json += eletroima_ligado ? "true" : "false";
    json += "}";
    server.send(200, "application/json", json);
  } else {
    server.send(400, "text/plain", "Parâmetro ausente");
  }
}

void handleStatus() {
  String json = "{";
  json += "\"posicao1\":" + String(posicao1) + ",";
  json += "\"posicao2\":" + String(posicao2) + ",";
  json += "\"posicao3\":" + String(posicao3) + ",";
  json += "\"eletroima_ligado\":";
  json += eletroima_ligado ? "true" : "false";
  json += "}";
  server.send(200, "application/json", json);
}

void handleReset() {
  posicao1 = 90;
  posicao2 = 90;
  posicao3 = 90;
  
  servo1.write(posicao1);
  servo2.write(posicao2);
  servo3.write(posicao3);
  
  // Desliga o eletroímã no reset por segurança
  eletroima_ligado = false;
  digitalWrite(ELETROIMÃ_PIN, LOW);
  
  String json = "{\"status\":\"ok\"}";
  server.send(200, "application/json", json);
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println("\n\n=== TechnoArm Iniciando ===");

  // Configura pino do eletroímã
  pinMode(ELETROIMÃ_PIN, OUTPUT);
  digitalWrite(ELETROIMÃ_PIN, LOW); // Inicia desligado
  Serial.println("✓ Eletroímã configurado (D0/GPIO16)");

  // Inicializa servos COM LIMITES DE PULSO do MG996R
  servo1.attach(SERVO1_PIN, SERVO_MIN_US, SERVO_MAX_US);
  servo2.attach(SERVO2_PIN, SERVO_MIN_US, SERVO_MAX_US);
  servo3.attach(SERVO3_PIN, SERVO_MIN_US, SERVO_MAX_US);

  // --- GARANTE INÍCIO EM 90° ---
  posicao1 = 90; posicao2 = 90; posicao3 = 90;
  servo1.write(posicao1);
  servo2.write(posicao2);
  servo3.write(posicao3);
  delay(500); // tempo para centralizar

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✓ WiFi Conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("\nAcesse o braço robótico em:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());

  // Rotas
  server.on("/", handleRoot);
  server.on("/mover", handleMover);
  server.on("/posicao", handlePosicao);
  server.on("/eletroima", handleEletroima);
  server.on("/status", handleStatus);
  server.on("/reset", handleReset);

  // Inicia servidor
  server.begin();
  Serial.println("\n✓ Servidor Web Iniciado!");
  Serial.println("=========================\n");
}

// ========== LOOP ==========
void loop() {
  server.handleClient();
}
