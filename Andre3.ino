#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SNMP_Agent.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <DHT.h>

// MAPEAMENTO DE HARDWARE
#define PINO_CORRENTE A0
#define PINO_PRESENCA D0
#define PINO_RELE D1
#define PINO_IR D2
#define PINO_TEMP D4
#define PINO_GAS D5

// VARIÁVEIS DA REDE
const char* ssid = "Mordor";
const char* password = "Andre1993";
IPAddress ip(192,168,0,99); 
IPAddress gateway(192,168,0,1); 
IPAddress subnet(255,255,255,0);

//const char* ssid = "Rede Wi-Fi de Luciano";
//const char* password = "Nupem@2016";
//IPAddress ip(172,16,7,96); 
//IPAddress gateway(172,16,254,249); 
//IPAddress subnet(255,255,0,0);

// OBJETOS
WiFiUDP udp;
SNMPAgent snmp = SNMPAgent("public");
IRsend irsend(PINO_IR);
DHT dht(PINO_TEMP, DHT22);

// VARIAVEIS GLOBAIS
int temperatura;
int umidade;
int presenca = 0;
int gases = 0;
int rele = 0;
int flag_temp; // Variável utilizada para mandar o sinal infravermelho para o A/C, caso necessário.
bool flag_leitura = false;
long int abertura;

uint16_t liga_ac[67] = {9000, 4500, 650, 550, 650, 1650, 600, 550, 650, 550,
                        600, 1650, 650, 550, 600, 1650, 650, 1650, 650, 1650,
                        600, 550, 650, 1650, 650, 1650, 650, 550, 600, 1650,
                        650, 1650, 650, 550, 650, 550, 650, 1650, 650, 550,
                        650, 550, 650, 550, 600, 550, 650, 550, 650, 550,
                        650, 1650, 600, 550, 650, 1650, 650, 1650, 650, 1650,
                        650, 1650, 650, 1650, 650, 1650, 600};

// TIMERS
os_timer_t tmr0;  // Timer para a leitura dos sensores de temperatura, presença e disparo do sinal de infravermelho

// CABEÇALHOS DE FUNÇÕES
void leitura(void*z);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(PINO_RELE, OUTPUT);
  pinMode(PINO_GAS, INPUT);
  pinMode(PINO_PRESENCA, INPUT);
    
  snmp.setUDP(&udp); // "Linka" o obejeto udp como  snmp
  snmp.begin();      // Inicia o SNMP
 
  // Criando os OIDs para o SNMP
  // snmpget -v 1 -c public 192.168.0.99 1.3.6.1.4.1.5.0  // COMANDO SNMP
  snmp.addIntegerHandler(".1.3.6.1.4.1.5.0", &temperatura);       // Adiciona a variável de "temperatura" ao SNMP 
  snmp.addIntegerHandler(".1.3.6.1.4.1.5.1", &umidade);           // Adiciona a variável de "umidade" ao SNMP
  snmp.addIntegerHandler(".1.3.6.1.4.1.5.2", &gases);             // Adiciona a variável de "gases" ao SNMP
  snmp.addIntegerHandler(".1.3.6.1.4.1.5.3", &presenca);          // Adiciona a variável que indica o "estado do presenca" // 
  snmp.addIntegerHandler(".1.3.6.1.4.1.5.4", &flag_temp, true);   // Adiciona a variável que indica a necessidade de "enviar um sinal por infravermelho"
  snmp.addIntegerHandler(".1.3.6.1.4.1.5.5", &rele, true);        // Adiciona a variável que realiza a abertura da porta
  
  dht.begin();
  snmp.sortHandlers();
  delay(2000);

  os_timer_setfn(&tmr0, leitura, NULL); // Indica a função que será chamada no estouro do timer
  os_timer_arm(&tmr0, 15000, true);     // Define o tempo de estouro do timer em 15000 milisegundos
}

void loop(){
  snmp.loop();
  
  if(snmp.setOccurred){
    // Aciona o relé para abertura da porta
    if(rele){
      digitalWrite(PINO_RELE, rele);
      abertura = millis();
    }

    if(millis() - abertura > 3000){
      rele = 0;
      digitalWrite(PINO_RELE, rele);
    }
    
    // Envia o sinal de infravermelho para ligar o ar condicionado
    if(flag_temp){
      irsend.sendRaw(liga_ac, 67, 38);
      Serial.println("Comando para o ar condicionado enviado!!!");
      flag_temp = 0;
    }
    snmp.resetSetOccurred();
  }

  if(flag_leitura){
    temperatura = dht.readTemperature(); // Leitura do sensor de temperatura
    umidade = dht.readHumidity();        // Leitura do sensor de umidade
    gases = digitalRead(PINO_GAS);       // Leitura do sensor de gases
    flag_leitura = false;
  }

  if(flag_temp){
    irsend.sendRaw(liga_ac, 67, 38);
    flag_temp = 0;
  }
  
  // Verificar o estado do sensor de presença
  presenca = (digitalRead(PINO_PRESENCA)) ? 1 : 0;
}

// ROTINA PARA ATUALIZAR OS VALORES A CADA 15s
void leitura(void*z){
  flag_leitura = true;
}
