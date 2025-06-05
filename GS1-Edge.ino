#include <LiquidCrystal_I2C.h> // Inclui a biblioteca LiquidCrystal_I2C para comunicação com o display LCD via I2C.
#include <RTClib.h> // Inclui a biblioteca RTClib para gerenciar o módulo de relógio de tempo real (RTC).
#include <EEPROM.h> // Inclui a biblioteca EEPROM para armazenar dados de forma persistente na memória EEPROM.
#include <Wire.h> // Inclui a biblioteca Wire para comunicação I2C.


#define TRIG 9 // Define o pino TRIG do sensor ultrassônico como 9.
#define ECHO 10 // Define o pino ECHO do sensor ultrassônico como 10.

// Declaração de variáveis para armazenar a duração do pulso e as distâncias.
long duracao;
float distanciaCm;
float distanciaM;
float distancia;
String estadoRio; // Variável para armazenar o estado atual do rio.
bool usarMetros; // Variável booleana para decidir se a distância será exibida em metros ou centímetros.

// Definição dos pinos para o buzzer e os LEDs indicadores.
int buzzer = 2;
int ledVerde = 7;
int ledAmarelo = 6;
int ledVermelho = 5;

// Definição dos endereços de memória EEPROM para salvar configurações.
const int distAdress = 0;           // Endereço para a configuração de unidade de distância.
const int fusoAdress = 1;           // Endereço para a configuração do fuso horário.
const int guardarAtualAdress = 5;   // Endereço para o ponteiro de início do log.
const int endAdress = 500;          // Endereço final da área de log na EEPROM.
const int tamanhoDeLog = sizeof(long) + sizeof(float); // Tamanho de cada entrada de log (timestamp + distância).

LiquidCrystal_I2C lcd(0x27, 20, 4); // Inicializa o objeto LCD com o endereço I2C (0x27), 20 colunas e 4 linhas.
RTC_DS1307 RTC; // Inicializa o objeto RTC para o módulo DS1307.

// Variáveis booleanas para controlar qual menu está ativo.
bool menuPrincipal = true;
bool menuDist = false;
bool menuFuso = false;
bool menuDataLogger = false;
String entradaUsuario = ""; // String para armazenar a entrada do usuário via Serial.

// Credenciais de administrador para acesso a funções restritas.
String usuarioAdm = "Admin";
String senhaAdm = "1234";
// Variáveis para controlar o estado da autenticação do usuário.
bool aguardandoUsuario = false;
bool aguardandoSenha = false;
String usuarioDigitado = "";

// Definição de caracteres personalizados para o display LCD (ícones de bode).
byte bode[] = {B00000, B10001, B11011, B11111, B10101, B11111, B01110, B00100};
byte chifreEsq[] = {B00000, B00111, B01100, B01001, B00110, B00000, B00000, B00000};
byte chifreDir[] = {B00000, B11100, B00110, B10010, B01100, B00000, B00000, B00000};

// Variáveis para armazenar o fuso horário e o offset UTC.
int fuso;
int UTC_OFFSET;

// Variável para armazenar o endereço de início da próxima gravação de log.
int startAdress;

void setup() {
  lcd.init(); // Inicializa o display LCD.
  lcd.backlight(); // Liga a luz de fundo do LCD.
  Serial.begin(9600); // Inicia a comunicação serial a 9600 bauds.
  EEPROM.begin(); // Inicia a EEPROM.
  RTC.begin(); // Inicia o módulo RTC.
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__))); // Ajusta a hora do RTC com base na data e hora da compilação.

  // Cria caracteres personalizados no LCD.
  lcd.createChar(0, chifreEsq);
  lcd.createChar(1, bode);
  lcd.createChar(2, chifreDir);

  // Configura os pinos do sensor ultrassônico como OUTPUT e INPUT.
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Configura os pinos do buzzer e LEDs como OUTPUT.
  pinMode(buzzer, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAmarelo, OUTPUT);
  pinMode(ledVermelho, OUTPUT);

  // Carrega as configurações de unidade de distância e fuso horário da EEPROM.
  EEPROM.get(distAdress, usarMetros);
  EEPROM.get(fusoAdress, fuso);

  // Define o offset UTC com o valor carregado.
  UTC_OFFSET = fuso;

  // Carrega o endereço de início do log da EEPROM.
  EEPROM.get(guardarAtualAdress, startAdress);
  // Verifica se o endereço de início do log é válido; caso contrário, redefine para o valor inicial.
  if (startAdress < 5 || startAdress > endAdress) {
    startAdress = 5;
  }

  
  animarLogo(2, 0, 100); // Exibe a animação do logo inicial.
  delay(1000); // Pequena pausa.
  animarBemVindo(5, 1, 100); // Exibe a animação de "Bem-Vindo!".
  delay(3000); // Pequena pausa.

  lcd.clear(); // Limpa o display LCD.
  mostrarMenuPrincipal(); // Exibe o menu principal.
}

void loop() {

  float somaDist = 0; // Variável para somar as leituras de distância.
 
  // Realiza 100 leituras para obter uma média mais precisa.
  for (int i = 0; i < 100; i++) {
    lerInput(); // Lê a entrada do usuário via Serial.

    // Gera um pulso no pino TRIG para iniciar a medição.
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    duracao = pulseIn(ECHO, HIGH); // Mede a duração do pulso de retorno no pino ECHO.
    float distanciaCmBruto = (duracao / 58.2); // Converte a duração em distância em centímetros.
    somaDist += distanciaCmBruto; // Adiciona a leitura à soma.
    delay(10); // Pequena pausa entre as leituras.
  }

  
  distanciaCm = somaDist / 100.0; // Calcula a distância média em centímetros.
  distanciaM = distanciaCm / 100.0; // Converte a distância média para metros.

  // Atribui a distância final com base na unidade de medida selecionada.
  if (usarMetros) {
    distancia = distanciaM;
  } else {
    distancia = distanciaCm;
  }

  // Verifica o nível da água e atualiza o estado do rio e os LEDs/buzzer.
  if (distanciaM >= 2) {
    // Rio em nível normal (verde).
    estadoRio = "Rio normal";
    digitalWrite(ledVerde, HIGH);
    digitalWrite(ledAmarelo, LOW);
    digitalWrite(ledVermelho, LOW);
    noTone(buzzer); // Desliga o buzzer.
  } else if (distanciaM < 2 && distanciaM > 1) {
    // Alerta de cheia (amarelo) com aviso sonoro.
    estadoRio = "Alerta de cheia";
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarelo, HIGH);
    digitalWrite(ledVermelho, LOW);
    tone(buzzer, 300); // Toca o buzzer em 300 Hz.
    delay(500);
    noTone(buzzer); // Desliga o buzzer após um tempo.
    gravarLog(distanciaCm); // Grava o log da distância.
  } else {
    // Risco de alagamento (vermelho) com aviso sonoro contínuo.
    estadoRio = "RISCO DE ALAGAMENTO";
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarelo, LOW);
    digitalWrite(ledVermelho, HIGH);
    tone(buzzer, 500); // Toca o buzzer em 500 Hz.
    gravarLog(distanciaCm); // Grava o log da distância.
  }

  // Exibe as informações da distância e estado do rio no LCD.
  exibirInfo(distancia, usarMetros, estadoRio);

}

// Função para animar a exibição do logo no LCD.
void animarLogo(int x, int y, int tempo) {
  // Posiciona o cursor no LCD.
  lcd.setCursor(x, y);

  // Escreve os caracteres personalizados do bode.
  lcd.write((byte)0);
  lcd.write((byte)1);
  lcd.write((byte)2);
  lcd.print(" ");

  // Escreve "Dev Goat" letra por letra com atraso.
  lcd.print("D"); delay(tempo);
  lcd.print("e"); delay(tempo);
  lcd.print("v"); delay(tempo);
  lcd.print(" "); delay(tempo);
  lcd.print("G"); delay(tempo);
  lcd.print("o"); delay(tempo);
  lcd.print("a"); delay(tempo);
  lcd.print("t"); delay(tempo);
  lcd.print(" ");

  // Escreve novamente os caracteres personalizados do bode.
  lcd.write((byte)0);
  lcd.write((byte)1);
  lcd.write((byte)2);
}

// Função para animar a mensagem de "Bem-Vindo!".
void animarBemVindo(int x, int y, int tempo) {
  // Posiciona o cursor no LCD.
  lcd.setCursor(x, y);

  // Escreve "Bem-Vindo!" letra por letra com atraso.
  lcd.print("B"); delay(tempo);
  lcd.print("e"); delay(tempo);
  lcd.print("m"); delay(tempo);
  lcd.print("-"); delay(tempo);
  lcd.print("V"); delay(tempo);
  lcd.print("i"); delay(tempo);
  lcd.print("n"); delay(tempo);
  lcd.print("d"); delay(tempo);
  lcd.print("o"); delay(tempo);
  lcd.print("!"); delay(tempo);
}

// Função para exibir o título no LCD.
void exibirTitulo() {
  lcd.setCursor(2, 0);
  lcd.print("Distancia do rio");
  lcd.setCursor(5, 1);
  lcd.print("ate o sensor");
}

// Função para exibir as informações da distância e estado do rio no LCD.
void exibirInfo(float distancia, bool usarMetros, String estadoRio) {
  // Limpa o display LCD.
  lcd.clear();
  // Exibe o título.
  exibirTitulo();

  // Exibe a distância com a unidade correta (metros ou centímetros).
  if (usarMetros) {
    lcd.setCursor(8, 2);
    lcd.print(distancia, 2); // 2 casas decimais para metros.
    lcd.print("m");
  } else {
    lcd.setCursor(8, 2);
    lcd.print(distancia, 0); // Sem casas decimais para centímetros.
    lcd.print("cm");
  }

  // Exibe o estado atual do rio.
  lcd.setCursor (0, 3);
  lcd.print(estadoRio);
}

// Função para ler a entrada do usuário via Serial.
void lerInput() {
  // Verifica se há dados disponíveis na Serial.
  if (Serial.available()) {
    // Lê um caractere da Serial.
    char c = Serial.read();

    // Se o caractere for uma quebra de linha ou retorno de carro, processa a entrada.
    if (c == '\n' || c == '\r') {
      if (entradaUsuario.length() > 0) {
        // Chama a função para tratar a opção digitada pelo usuário.
        tratarOpcao(entradaUsuario);
        // Limpa a string de entrada do usuário.
        entradaUsuario = "";
      }
    } else {
      // Adiciona o caractere lido à string de entrada do usuário.
      entradaUsuario += c;
    }
  }
}

// Função para tratar a opção selecionada pelo usuário.
void tratarOpcao(String comando) {
  // Remove espaços em branco no início e fim do comando.
  comando.trim();

  // Verifica em qual menu o usuário está e processa o comando.
  if (menuPrincipal) {
    if (comando == "1") {
      // Entra no menu de configurações de distância.
      menuDist = true;
      menuFuso = false;
      menuPrincipal = false;
      menuDataLogger = false;
      mostrarMenuDist();
    } else if (comando == "2") {
      // Entra no menu de configurações de fuso horário.
      menuDist = false;
      menuFuso = true;
      menuPrincipal = false;
      menuDataLogger = false;
      mostrarMenuFuso();
    } else if (comando == "3") {
      // Entra no menu de Data Logger.
      menuDist = false;
      menuFuso = false;
      menuPrincipal = false;
      menuDataLogger = true;
      mostrarMenuDataLogger();
    } else {
      // Mensagem de opção inválida no menu principal.
      Serial.println("Opcao invalida no Menu Principal.");
    }
  }
  // Se estiver no menu de configurações de distância.
  else if (menuDist) {
    if (comando == "1") {
      // Define a unidade para centímetros e salva na EEPROM.
      usarMetros = false;
      EEPROM.put(distAdress, usarMetros);
      Serial.println("Distancia sera exibida em centimetros.");
      Serial.println("Reinicie o sistema, para que as alteracoes sejam salvas!");
    } else if (comando == "2") {
      // Define a unidade para metros e salva na EEPROM.
      usarMetros = true;
      EEPROM.put(distAdress, usarMetros);
      Serial.println("Distancia sera exibida em metros.");
      Serial.println("Reinicie o sistema para que as alteracoes sejam salvas!");
    } else if (comando.equalsIgnoreCase("voltar")) {
      // Volta para o menu principal.
      menuDist = false;
      menuFuso = false;
      menuDataLogger = false;
      menuPrincipal = true;
      mostrarMenuPrincipal();
    } else {
      // Mensagem de opção inválida no menu de distância.
      Serial.println("Opcao invalida no Menu Distancia.");
    }
  }
  // Se estiver no menu de configurações de fuso horário.
  else if (menuFuso) {
    if (comando.equalsIgnoreCase("voltar")) {
      // Volta para o menu principal.
      menuDist = false;
      menuFuso = false;
      menuDataLogger = false;
      menuPrincipal = true;
      mostrarMenuPrincipal();
    } else {
      // Converte o comando para um número inteiro e atualiza o fuso horário.
      fuso = comando.toInt();
      EEPROM.put(fusoAdress, fuso); // Salva o novo fuso horário na EEPROM.
      UTC_OFFSET = fuso; // Atualiza o offset UTC.
      Serial.print("Fuso horario atualizado para: UTC");
      if (fuso >= 0) { Serial.print("+"); }
      Serial.println(fuso);
      Serial.println("Reinicie o sistema para que as alteracoes sejam salvas!");
    }
  }
  // Se estiver no menu de Data Logger.
  else if (menuDataLogger) {
    // Se estiver aguardando o nome de usuário.
    if (aguardandoUsuario) {
      usuarioDigitado = comando;
      if (usuarioDigitado == usuarioAdm) {
        Serial.println("Digite a senha:");
        aguardandoUsuario = false;
        aguardandoSenha = true; // Agora aguarda a senha.
      } else {
        Serial.println("Usuario incorreto.");
        aguardandoUsuario = false;
        mostrarMenuDataLogger(); // Volta a exibir o menu Data Logger.
      }
    }
    // Se estiver aguardando a senha.
    else if (aguardandoSenha) {
      if (comando == senhaAdm) {
        Serial.println("Limpando Data Logger...");
        // Limpa a área da EEPROM reservada para logs.
        for (int addr = 5; addr <= endAdress; addr++) {
          EEPROM.write(addr, 0xFF); // Escreve 0xFF (vazio) em cada byte.
        }
        startAdress = 5; // Reseta o ponteiro de início do log.
        EEPROM.put(guardarAtualAdress, startAdress); // Salva o novo ponteiro na EEPROM.
        Serial.println("Data Logger limpo!");
      } else {
        Serial.println("Senha incorreta.");
      }
      aguardandoSenha = false;
      mostrarMenuDataLogger(); // Volta a exibir o menu Data Logger.
    }
    // Opções normais do menu Data Logger.
    else if (comando == "1") {
      Serial.println("\n--- Log de Distancia ---");
      Serial.println("Data/Hora\t\tDistancia (cm)");

      // Loop para ler e exibir cada entrada de log.
      for (int adress = 5; adress <= endAdress; adress += tamanhoDeLog) {
        long timeStamp;
        float dist;

        // Lê o timestamp do log.
        EEPROM.get(adress, timeStamp);
        // Verifica se a entrada não está vazia.
        if (timeStamp != 0xFFFFFFFF && timeStamp != 0) {
          // Lê a distância do log.
          EEPROM.get(adress + sizeof(long), dist);
          
          // Converte o timestamp para um objeto DateTime.
          DateTime dt = DateTime(timeStamp);
          // Exibe a data e hora formatadas.
          Serial.print(dt.timestamp(DateTime::TIMESTAMP_FULL));
          Serial.print("\t");
          // Exibe a distância.
          Serial.print(dist, 0);
          Serial.println("cm");
        }
      }
      Serial.println("--- Fim do Log ---");
    } else if (comando == "2") {
      Serial.println("Digite o usuario:");
      aguardandoUsuario = true; // Inicia o processo de autenticação.
    } else if (comando.equalsIgnoreCase("voltar")) {
      // Volta para o menu principal.
      menuDist = false;
      menuFuso = false;
      menuDataLogger = false;
      menuPrincipal = true;
      mostrarMenuPrincipal();
    } else {
      // Mensagem de opção inválida no menu Data Logger.
      Serial.println("Opcao invalida no Menu Data Logger.");
    }
  }
}

// Função para gravar dados de distância no log da EEPROM.
void gravarLog(float distParaLog) {
  // Verifica se há espaço disponível na EEPROM para gravar o log.
  if (startAdress + tamanhoDeLog <= endAdress) {
    // Obtém a data e hora atuais do RTC.
    DateTime now = RTC.now();
    // Calcula o timestamp Unix e aplica o offset de fuso horário.
    long timeStamp = now.unixtime() + (UTC_OFFSET * 3600);

    // Grava o timestamp e a distância na EEPROM.
    EEPROM.put(startAdress, timeStamp);
    EEPROM.put(startAdress + sizeof(long), distParaLog);

    // Atualiza o endereço de início para a próxima gravação.
    startAdress += tamanhoDeLog;
    EEPROM.put(guardarAtualAdress, startAdress); // Salva o novo endereço na EEPROM.
  } else {
    // Mensagem de EEPROM cheia.
    Serial.println("EEPROM cheia! Apague os logs para gravar novos dados.");
  }
}

// Funções para exibir os diferentes menus na Serial.

void mostrarMenuPrincipal() {
  Serial.println("\n=== Sistema de controle ===");
  Serial.println("[1] Configuracoes de distancia");
  Serial.println("[2] Configuracoes de fuso horario");
  Serial.println("[3] Data Logger");
  Serial.print("Selecione uma opcao: ");
}

void mostrarMenuDist() {
  Serial.println("\n=== Configuracoes de distancia ===");
  Serial.println("[1] Exibir distancia em centimetros");
  Serial.println("[2] Exibir distancia em metros");
  Serial.println("[Voltar] Voltar ao menu inicial");
  Serial.print("Selecione uma opcao: ");
}

void mostrarMenuFuso() {
  Serial.println("\n=== Configuracoes de fuso ===");
  Serial.print("Fuso horario atual: UTC");
  if(UTC_OFFSET >= 0) { Serial.print("+"); }
  Serial.println(UTC_OFFSET);
  Serial.println("Digite o fuso horario (ex: -3, 0, 3 ...)");
  Serial.println("[Voltar] Voltar ao menu inicial");
  Serial.print("Digite o fuso ou 'voltar': ");
}

void mostrarMenuDataLogger() {
  Serial.println("\n=== Data Logger ===");
  Serial.println("[1] Exibir Data Log");
  Serial.println("[2] Apagar Data Logger (requer login)");
  Serial.println("[Voltar] Voltar ao menu inicial");
  Serial.print("Selecione uma opcao: ");
}
