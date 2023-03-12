#include <DHTesp.h>
#include <PubSubClient.h>
#include <WiFi.h>

#define PIN_LED 5 // GPIO que está ligado o LED

/* Configura os tópicos do MQTT */
#define TOPIC_SUBSCRIBE_LED       "topic_on_off_led"
#define TOPIC_PUBLISH_TESTE       "MeuTopico"

#define PUBLISH_DELAY 2000   // Atraso da publicação (2 segundos)

#define ID_MQTT "esp32_mqtt" // id mqtt (para identificação de sessão)

// IMPORTANTE: Este deve ser único no broker, ou seja, se um client MQTT
// tentar entrar com o mesmo id de outro já conectado ao broker,
// o broker irá fechar a conexão de um deles.

/* Variaveis, constantes e objetos globais */

const char *SSID = "Wokwi-GUEST"; // SSID / nome da rede WI-FI que deseja se conectar
const char *PASSWORD = "";        // Senha da rede WI-FI que deseja se conectar

// URL do broker MQTT que se deseja utilizar
const char *BROKER_MQTT = "broker.mqttdashboard.com";

int BROKER_PORT = 1883; // Porta do Broker MQTT

unsigned long publishUpdate;

// Variáveis e objetos globais
WiFiClient espClient;         // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

void initWiFi(void);
void initMQTT(void);
void callbackMQTT(char *topic, byte *payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void checkWiFIAndMQTT(void);

/* Inicializa e conecta-se na rede WI-FI desejada */
void initWiFi(void)
{
  delay(10);

  reconnectWiFi();
}

/* Inicializa os parâmetros de conexão MQTT(endereço do broker, porta e seta
  função de callback) */
void initMQTT(void)
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); // Informa qual broker e porta deve ser conectado
  //MQTT.setCallback(callbackMQTT);           // Atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

/* Função de callback  esta função é chamada toda vez que uma informação
   de um dos tópicos subescritos chega */
/*void callbackMQTT(char *topic, byte *payload, unsigned int length)
{
  String msg;

  // Obtem a string do payload recebido
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }

  //Serial.printf("Chegou a seguinte string via MQTT: %s do topico: %s\n", msg, topic);

  // Toma ação dependendo da string recebida
  if (msg.equals("1")) {
    //Serial.print("LED ligado por comando MQTT");
  }

  if (msg.equals("0")) {
    //Serial.print("LED desligado por comando MQTT");
  }

  if (msg.equals("2")) {
    //Serial.print("LED alternado por comando MQTT");
  }
}*/

/* Reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
   em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito. */
void reconnectMQTT(void)
{
  while (!MQTT.connected()) {
    if (MQTT.connect(ID_MQTT)) {
      //MQTT.subscribe(TOPIC_SUBSCRIBE_LED);
    } else {
      delay(2000);
    }
  }
}

/* Verifica o estado das conexões WiFI e ao broker MQTT.
  Em caso de desconexão (qualquer uma das duas), a conexão é refeita. */
void checkWiFIAndMQTT(void)
{
  if (!MQTT.connected())
    reconnectMQTT(); // se não há conexão com o Broker, a conexão é refeita

  reconnectWiFi(); // se não há conexão com o WiFI, a conexão é refeita
}

void reconnectWiFi(void)
{
  // se já está conectado a rede WI-FI, nada é feito.
  // Caso contrário, são efetuadas tentativas de conexão
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    //Serial.print(".");
  }
}

void inicializarMQTT()
{
  //Serial.begin(115200);

  // Inicializa a conexao Wi-Fi
  initWiFi();

  // Inicializa a conexao ao broker MQTT
  initMQTT();
}

void atualizarMQTT()
{
  /* Repete o ciclo após 2 segundos */
  if ((millis() - publishUpdate) >= PUBLISH_DELAY) {
    publishUpdate = millis();
    // Verifica o funcionamento das conexões WiFi e ao broker MQTT
    checkWiFIAndMQTT();

    // Keep-alive da comunicação com broker MQTT
    MQTT.loop();
  }
}

void publicarNoTopicoMQTT(String topico, String informacao){
  const char* topicoChar = topico.c_str();
  const char* informacaoChar = informacao.c_str();
  MQTT.publish(topicoChar, informacaoChar);
}