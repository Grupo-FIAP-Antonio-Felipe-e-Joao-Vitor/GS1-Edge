#include <LiquidCrystal_I2C.h> // Biblioteca para controle do display LCD via I2C
#include <RTClib.h>            // Biblioteca para uso do RTC DS1307
#include <EEPROM.h>            // Biblioteca para leitura/gravação na memória EEPROM
#include <Wire.h>              // Comunicação I2C

// Pinos do sensor ultrassônico
#define TRIG 9 // Pino TRIG (Trigger) do sensor ultrassônico, envia o pulso sonoro
#define ECHO 10 // Pino ECHO (Echo) do sensor ultrassônico, recebe o pulso sonoro refletido

// Variáveis de medição
long duracao; // Armazena a duração do pulso de ECHO (tempo que o som leva para ir e voltar)
float distanciaCm; // Armazena a distância medida em centímetros
float distanciaM; // Armazena a distância medida em metros
float distancia; // Armazena a distância final, que pode ser em cm ou m, dependendo da configuração
String estadoRio; // Armazena o estado atual do rio (Normal, Alerta, Risco)
bool usarMetros; // Flag booleana para indicar se a distância deve ser exibida em metros (true) ou centímetros (false)

// Pinos de atuadores
int buzzer = 2;      // Pino digital conectado ao buzzer (para alertas sonoros)
int ledVerde = 7;    // Pino digital conectado ao LED verde (indica rio normal)
int ledAmarelo = 6;  // Pino digital conectado ao LED amarelo (indica alerta de cheia)
int ledVermelho = 5; // Pino digital conectado ao LED vermelho (indica risco de alagamento)

// Endereços na EEPROM para diferentes configurações/dados
const int distAdress = 0;       // Endereço na EEPROM para guardar a configuração da unidade de distância (metros/centímetros)
const int fusoAdress = 1;       // Endereço na EEPROM para guardar a configuração do fuso horário
const int guardarAtualAdress = 5; // Endereço onde será guardado o próximo endereço de escrita disponível na EEPROM para o Data Logger
const int endAdress = 500;      // Limite da EEPROM para o Data Logger (endereço final disponível para gravação de logs)
const int tamanhoDeLog = sizeof(long) + sizeof(float); // Tamanho de cada entrada de log: timestamp (long) + distância (float)

// Objetos do display e do RTC
LiquidCrystal_I2C lcd(0x27, 20, 4); // Cria um objeto LCD com endereço I2C 0x27, 20 colunas e 4 linhas
RTC_DS1307 RTC; // Cria um objeto para o módulo RTC DS1307

// Flags para controle de menus
bool menuPrincipal = true;      // Flag para indicar se o menu principal está ativo
bool menuDist = false;          // Flag para indicar se o menu de distância está ativo
bool menuFuso = false;          // Flag para indicar se o menu de fuso horário está ativo
bool menuDataLogger = false;    // Flag para indicar se o menu do Data Logger está ativo
String entradaUsuario = "";     // Armazena a entrada de texto do usuário via serial

// Credenciais para autenticação (para acessar funções restritas como apagar logs)
String usuarioAdm = "Admin";    // Nome de usuário para autenticação
String senhaAdm = "1234";       // Senha para autenticação
bool aguardandoUsuario = false; // Flag para indicar se o sistema está aguardando a entrada do usuário
bool aguardandoSenha = false;   // Flag para indicar se o sistema está aguardando a entrada da senha
String usuarioDigitado = "";    // Armazena o usuário digitado pelo usuário

// Caracteres personalizados (bicho com chifres) para animação no LCD
// São arrays de bytes que representam a forma dos caracteres personalizados
byte bode[] = {B00000, B10001, B11011, B11111, B10101, B11111, B01110, B00100};
byte chifreEsq[] = {B00000, B00111, B01100, B01001, B00110, B00000, B00000, B00000};
byte chifreDir[] = {B00000, B11100, B00110, B10010, B01100, B00000, B00000, B00000};

// Variáveis de fuso horário
int fuso;         // Armazena o valor do fuso horário configurado
int UTC_OFFSET;   // Armazena o offset de UTC com base no fuso horário configurado

// Endereço atual de gravação de log
int startAdress; // Próximo endereço disponível na EEPROM para iniciar uma nova gravação de log

void setup() {
  // Inicializações básicas
  lcd.init();       // Inicializa o display LCD
  lcd.backlight();  // Liga a luz de fundo do LCD
  Serial.begin(9600); // Inicia a comunicação serial com um baud rate de 9600
  EEPROM.begin();   // Inicializa a EEPROM
  RTC.begin();      // Inicializa o módulo RTC
  // Ajusta o RTC com a data e hora da compilação do programa.
  // Isso garante que o RTC comece com uma hora válida se não tiver sido ajustado manualmente.
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__))); 

  // Criação dos caracteres especiais no LCD
  // Associa os arrays de bytes definidos anteriormente a códigos de caracteres personalizados (0, 1, 2)
  lcd.createChar(0, chifreEsq);
  lcd.createChar(1, bode);
  lcd.createChar(2, chifreDir);

  // Configuração dos pinos
  // Define os pinos do sensor ultrassônico e dos LEDs/buzzer como OUTPUT ou INPUT
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAmarelo, OUTPUT);
  pinMode(ledVermelho, OUTPUT);

  // Recupera configurações salvas na EEPROM
  // Lê os valores salvos anteriormente na EEPROM e os atribui às variáveis
  EEPROM.get(distAdress, usarMetros); // Recupera a preferência de unidade de medida
  EEPROM.get(fusoAdress, fuso);       // Recupera o fuso horário configurado
  UTC_OFFSET = fuso;                  // Atualiza o offset de UTC

  // Recupera o próximo endereço de escrita para o Data Logger
  EEPROM.get(guardarAtualAdress, startAdress);
  // Se o endereço recuperado for inválido (fora dos limites), reinicia para o primeiro endereço válido
  if (startAdress < 5 || startAdress > endAdress) {
    startAdress = 5;
  }

  // Animações iniciais no LCD
  // Exibe animações de boas-vindas no LCD antes de entrar no loop principal
  animarLogo(2, 0, 100);    // Exibe o logo "Dev Goat"
  delay(1000);              // Pausa de 1 segundo
  animarBemVindo(5, 1, 100); // Exibe "Bem-Vindo!"
  delay(3000);              // Pausa de 3 segundos
  lcd.clear();              // Limpa o display
  mostrarMenuPrincipal();   // Exibe o menu principal na porta serial
}

void loop() {
  // Média de 100 leituras para melhor precisão
  float somaDist = 0; // Variável para acumular a soma das distâncias
  for (int i = 0; i < 100; i++) {
    lerInput(); // Chama a função para verificar se há entrada serial do usuário
    // Sequência para acionar o sensor ultrassônico
    digitalWrite(TRIG, LOW);      // Garante que o pino TRIG esteja em LOW
    delayMicroseconds(2);         // Pequena pausa
    digitalWrite(TRIG, HIGH);     // Envia um pulso HIGH no pino TRIG por 10 microssegundos
    delayMicroseconds(10);        // Duração do pulso
    digitalWrite(TRIG, LOW);      // Desliga o pulso

    // Calcula a duração do pulso de ECHO e a distância em centímetros
    duracao = pulseIn(ECHO, HIGH); // Mede a duração do pulso HIGH no pino ECHO
    float distanciaCmBruto = (duracao / 58.2); // Converte a duração para distância em centímetros (fórmula padrão)
    somaDist += distanciaCmBruto; // Acumula a distância para o cálculo da média
    delay(10); // Pequena pausa entre as leituras
  }

  // Cálculo das distâncias média
  distanciaCm = somaDist / 100.0;     // Calcula a distância média em centímetros
  distanciaM = distanciaCm / 100.0;   // Converte a distância média para metros
  // Atribui a distância final com base na preferência do usuário (metros ou centímetros)
  distancia = usarMetros ? distanciaM : distanciaCm;

  // Verificação do estado do rio e acionamento dos indicadores (LEDs e Buzzer)
  if (distanciaM >= 2) {
    estadoRio = "Rio normal";           // Se a distância for maior ou igual a 2 metros
    digitalWrite(ledVerde, HIGH);       // Liga o LED verde
    digitalWrite(ledAmarelo, LOW);      // Desliga o LED amarelo
    digitalWrite(ledVermelho, LOW);     // Desliga o LED vermelho
    noTone(buzzer);                     // Desliga qualquer som do buzzer
  } else if (distanciaM < 2 && distanciaM > 1) {
    estadoRio = "Alerta de cheia";      // Se a distância estiver entre 1 e 2 metros
    digitalWrite(ledVerde, LOW);        // Desliga o LED verde
    digitalWrite(ledAmarelo, HIGH);     // Liga o LED amarelo
    digitalWrite(ledVermelho, LOW);     // Desliga o LED vermelho
    tone(buzzer, 300);                  // Emite um som no buzzer com frequência de 300Hz
    delay(500);                         // Pausa de 0.5 segundos
    noTone(buzzer);                     // Desliga o som do buzzer
    gravarLog(distanciaCm);             // Grava a distância no log (quando há alerta)
  } else {
    estadoRio = "RISCO DE ALAGAMENTO";  // Se a distância for menor ou igual a 1 metro
    digitalWrite(ledVerde, LOW);        // Desliga o LED verde
    digitalWrite(ledAmarelo, LOW);      // Desliga o LED amarelo
    digitalWrite(ledVermelho, HIGH);    // Liga o LED vermelho
    tone(buzzer, 500);                  // Emite um som no buzzer com frequência de 500Hz (mais intenso)
    gravarLog(distanciaCm);             // Grava a distância no log (quando há risco)
  }

  // Exibe as informações atualizadas no display LCD
  exibirInfo(distancia, usarMetros, estadoRio);
}

// Funções de animação no display
void animarLogo(int x, int y, int tempo) {
  // Move o cursor para a posição (x, y) no LCD
  lcd.setCursor(x, y);
  // Imprime os caracteres personalizados para formar o logo "Dev Goat"
  lcd.write((byte)0); // Chifre esquerdo
  lcd.write((byte)1); // Corpo do bode
  lcd.write((byte)2); // Chifre direito
  lcd.print(" Dev Goat "); // Texto " Dev Goat "
  lcd.write((byte)0); // Repete o chifre esquerdo
  lcd.write((byte)1); // Repete o corpo do bode
  lcd.write((byte)2); // Repete o chifre direito
}

void animarBemVindo(int x, int y, int tempo) {
  // Move o cursor para a posição (x, y) no LCD
  lcd.setCursor(x, y);
  // Imprime a mensagem "Bem-Vindo!"
  lcd.print("Bem-Vindo!");
}

// Exibe título e estado do rio no LCD
void exibirTitulo() {
  // Define o cursor e imprime o título do display
  lcd.setCursor(2, 0);
  lcd.print("Distancia do rio");
  lcd.setCursor(5, 1);
  lcd.print("ate o sensor");
}

void exibirInfo(float distancia, bool usarMetros, String estadoRio) {
  lcd.clear(); // Limpa o display LCD antes de exibir novas informações
  exibirTitulo(); // Exibe o título fixo no display

  lcd.setCursor(8, 2); // Define a posição do cursor para exibir a distância
  if (usarMetros) {
    lcd.print(distancia, 2); // Imprime a distância com 2 casas decimais (em metros)
    lcd.print("m");          // Adiciona a unidade "m"
  } else {
    lcd.print(distancia, 0); // Imprime a distância sem casas decimais (em centímetros)
    lcd.print("cm");         // Adiciona a unidade "cm"
  }

  lcd.setCursor(0, 3);   // Define a posição do cursor para exibir o estado do rio
  lcd.print(estadoRio);  // Imprime a string que descreve o estado do rio
}

// Leitura da entrada serial
void lerInput() {
  if (Serial.available()) { // Verifica se há dados disponíveis na porta serial
    char c = Serial.read(); // Lê um caractere da porta serial

    if (c == '\n' || c == '\r') { // Se o caractere for uma quebra de linha ou retorno de carro
      if (entradaUsuario.length() > 0) { // Se a string de entrada do usuário não estiver vazia
        tratarOpcao(entradaUsuario); // Chama a função para tratar a opção digitada
        entradaUsuario = "";         // Limpa a string de entrada para a próxima leitura
      }
    } else {
      entradaUsuario += c; // Adiciona o caractere lido à string de entrada do usuário
    }
  }
}

// Trata comandos enviados via serial conforme o menu ativo
void tratarOpcao(String comando) {
  comando.trim(); // Remove espaços em branco do início e fim da string de comando

  if (menuPrincipal) { // Se o menu principal estiver ativo
    // Navegação entre menus
    if (comando == "1") { // Se o comando for "1"
      menuDist = true;       // Ativa o menu de distância
      menuPrincipal = false; // Desativa o menu principal
      mostrarMenuDist();     // Exibe o menu de distância na serial
    } else if (comando == "2") { // Se o comando for "2"
      menuFuso = true;       // Ativa o menu de fuso horário
      menuPrincipal = false; // Desativa o menu principal
      mostrarMenuFuso();     // Exibe o menu de fuso horário na serial
    } else if (comando == "3") { // Se o comando for "3"
      menuDataLogger = true;     // Ativa o menu do Data Logger
      menuPrincipal = false;     // Desativa o menu principal
      mostrarMenuDataLogger();   // Exibe o menu do Data Logger na serial
    } else {
      Serial.println("Opcao invalida no Menu Principal."); // Mensagem de erro para opção inválida
    }
  }
  else if (menuDist) { // Se o menu de distância estiver ativo
    // Configura unidade de medida
    if (comando == "1") { // Se o comando for "1"
      usarMetros = false; // Define para usar centímetros
      EEPROM.put(distAdress, usarMetros); // Salva a preferência na EEPROM
      Serial.println("Distancia sera exibida em centimetros."); // Confirmação na serial
    } else if (comando == "2") { // Se o comando for "2"
      usarMetros = true; // Define para usar metros
      EEPROM.put(distAdress, usarMetros); // Salva a preferência na EEPROM
      Serial.println("Distancia sera exibida em metros."); // Confirmação na serial
    } else if (comando.equalsIgnoreCase("voltar")) { // Se o comando for "voltar" (ignorando maiúsculas/minúsculas)
      menuDist = false;          // Desativa o menu de distância
      menuPrincipal = true;      // Ativa o menu principal
      mostrarMenuPrincipal();    // Exibe o menu principal na serial
    } else {
      Serial.println("Opcao invalida no Menu Distancia."); // Mensagem de erro para opção inválida
    }
  }
  else if (menuFuso) { // Se o menu de fuso horário estiver ativo
    // Configura fuso horário
    if (comando.equalsIgnoreCase("voltar")) { // Se o comando for "voltar"
      menuFuso = false;          // Desativa o menu de fuso
      menuPrincipal = true;      // Ativa o menu principal
      mostrarMenuPrincipal();    // Exibe o menu principal na serial
    } else {
      fuso = comando.toInt(); // Converte a string do comando para um inteiro (valor do fuso)
      EEPROM.put(fusoAdress, fuso); // Salva o fuso horário na EEPROM
      UTC_OFFSET = fuso;          // Atualiza o offset de UTC
      Serial.print("Fuso horario atualizado para: UTC"); // Confirmação na serial
      if (fuso >= 0) Serial.print("+"); // Adiciona "+" se o fuso for positivo ou zero
      Serial.println(fuso);               // Exibe o valor do fuso
    }
  }
  else if (menuDataLogger) { // Se o menu do Data Logger estiver ativo
    // Gerenciamento do log de dados
    if (aguardandoUsuario) { // Se estiver aguardando a entrada do usuário
      usuarioDigitado = comando; // Armazena o usuário digitado
      if (usuarioDigitado == usuarioAdm) { // Se o usuário digitado for o usuário admin
        Serial.println("Digite a senha:"); // Pede a senha
        aguardandoSenha = true; // Ativa a flag para aguardar a senha
      } else {
        Serial.println("Usuario incorreto."); // Mensagem de usuário incorreto
        mostrarMenuDataLogger(); // Volta a exibir o menu do Data Logger
      }
      aguardandoUsuario = false; // Desativa a flag de aguardar usuário
    } else if (aguardandoSenha) { // Se estiver aguardando a senha
      if (comando == senhaAdm) { // Se a senha digitada for a senha admin
        // Apaga logs: preenche a área de log da EEPROM com 0xFF (valor padrão de uma EEPROM vazia)
        for (int addr = 5; addr <= endAdress; addr++) {
          EEPROM.write(addr, 0xFF);
        }
        startAdress = 5; // Reinicia o endereço de escrita para o início da área de log
        EEPROM.put(guardarAtualAdress, startAdress); // Salva o novo endereço inicial na EEPROM
        Serial.println("Data Logger limpo!"); // Confirmação de limpeza
      } else {
        Serial.println("Senha incorreta."); // Mensagem de senha incorreta
      }
      aguardandoSenha = false; // Desativa a flag de aguardar senha
      mostrarMenuDataLogger(); // Volta a exibir o menu do Data Logger
    } else if (comando == "1") { // Se o comando for "1" (Exibir Data Log)
      Serial.println("\n--- Log de Distancia ---"); // Cabeçalho do log
      // Itera sobre os endereços da EEPROM para ler os logs
      for (int adress = 5; adress <= endAdress; adress += tamanhoDeLog) {
        long timeStamp; // Variável para armazenar o timestamp
        float dist;     // Variável para armazenar a distância

        EEPROM.get(adress, timeStamp); // Lê o timestamp do endereço atual
        // Verifica se o timestamp não é 0xFFFFFFFF (vazio) e não é 0 (também pode indicar vazio ou erro)
        if (timeStamp != 0xFFFFFFFF && timeStamp != 0) {
          EEPROM.get(adress + sizeof(long), dist); // Lê a distância do endereço seguinte ao timestamp
          DateTime dt = DateTime(timeStamp); // Cria um objeto DateTime a partir do timestamp
          Serial.print(dt.timestamp(DateTime::TIMESTAMP_FULL)); // Imprime o timestamp formatado
          Serial.print("\t"); // Tabulação
          Serial.print(dist, 0); // Imprime a distância sem casas decimais
          Serial.println("cm"); // Adiciona a unidade "cm"
        }
      }
      Serial.println("--- Fim do Log ---"); // Rodapé do log
    } else if (comando == "2") { // Se o comando for "2" (Apagar Data Logger)
      Serial.println("Digite o usuario:"); // Pede o usuário
      aguardandoUsuario = true; // Ativa a flag para aguardar o usuário
    } else if (comando.equalsIgnoreCase("voltar")) { // Se o comando for "voltar"
      menuDataLogger = false;    // Desativa o menu do Data Logger
      menuPrincipal = true;      // Ativa o menu principal
      mostrarMenuPrincipal();    // Exibe o menu principal na serial
    } else {
      Serial.println("Opcao invalida no Menu Data Logger."); // Mensagem de erro para opção inválida
    }
  }
}

// Salva um log na EEPROM com timestamp e distância
void gravarLog(float distParaLog) {
  // Verifica se ainda há espaço disponível na EEPROM para gravar o log
  if (startAdress + tamanhoDeLog <= endAdress) {
    DateTime now = RTC.now(); // Obtém a data e hora atual do RTC
    // Calcula o timestamp Unix (segundos desde 01/01/2000) e aplica o offset do fuso horário
    long timeStamp = now.unixtime() + (UTC_OFFSET * 3600); 

    EEPROM.put(startAdress, timeStamp); // Grava o timestamp na EEPROM no endereço atual
    EEPROM.put(startAdress + sizeof(long), distParaLog); // Grava a distância logo após o timestamp

    startAdress += tamanhoDeLog; // Avança o ponteiro do próximo endereço de escrita
    EEPROM.put(guardarAtualAdress, startAdress); // Salva o novo endereço de escrita na EEPROM
  } else {
    // Se a EEPROM estiver cheia, exibe uma mensagem de aviso
    Serial.println("EEPROM cheia! Apague os logs para gravar novos dados.");
  }
}

// Funções para exibir menus via Serial
void mostrarMenuPrincipal() {
  Serial.println("\n=== Sistema de controle ==="); // Título do menu principal
  Serial.println("[1] Configuracoes de distancia"); // Opção 1: Configurações de distância
  Serial.println("[2] Configuracoes de fuso horario"); // Opção 2: Configurações de fuso horário
  Serial.println("[3] Data Logger"); // Opção 3: Data Logger
  Serial.print("Selecione uma opcao: "); // Solicita que o usuário selecione uma opção
}

void mostrarMenuDist() {
  Serial.println("\n=== Configuracoes de distancia ==="); // Título do menu de distância
  Serial.println("[1] Exibir distancia em centimetros"); // Opção 1: Centímetros
  Serial.println("[2] Exibir distancia em metros"); // Opção 2: Metros
  Serial.println("[Voltar] Voltar ao menu inicial"); // Opção para voltar
  Serial.print("Selecione uma opcao: "); // Solicita que o usuário selecione uma opção
}

void mostrarMenuFuso() {
  Serial.println("\n=== Configuracoes de fuso ==="); // Título do menu de fuso
  Serial.print("Fuso horario atual: UTC"); // Exibe o fuso horário atual
  if(UTC_OFFSET >= 0) Serial.print("+"); // Adiciona "+" se o fuso for positivo ou zero
  Serial.println(UTC_OFFSET); // Exibe o valor do fuso
  Serial.println("Digite o fuso horario (ex: -3, 0, 3 ...)"); // Instruções para digitar o fuso
  Serial.println("[Voltar] Voltar ao menu inicial"); // Opção para voltar
  Serial.print("Digite o fuso ou 'voltar': "); // Solicita a entrada do usuário
}

void mostrarMenuDataLogger() {
  Serial.println("\n=== Data Logger ==="); // Título do menu do Data Logger
  Serial.println("[1] Exibir Data Log"); // Opção 1: Exibir logs
  Serial.println("[2] Apagar Data Logger (requer login)"); // Opção 2: Apagar logs (requer autenticação)
  Serial.println("[Voltar] Voltar ao menu inicial"); // Opção para voltar
  Serial.print("Selecione uma opcao: "); // Solicita que o usuário selecione uma opção
}
