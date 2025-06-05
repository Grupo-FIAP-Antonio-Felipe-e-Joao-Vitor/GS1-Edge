# 🌊 Monitoramento de Nível de Rio com Arduino

Este projeto Arduino monitora o nível de um rio usando um sensor ultrassônico e fornece alertas visuais e sonoros com base na distância medida. Ele também inclui um display LCD para mostrar as informações em tempo real e um sistema de log de dados persistente utilizando a EEPROM.

---

## 🚀 Funcionalidades

- **Medição de Distância:** Sensor ultrassônico (HC-SR04) mede continuamente a distância entre o sensor e a superfície da água.
- **Alertas Visuais:**
  - 🟢 Verde: Nível normal.
  - 🟡 Amarelo: Alerta de cheia.
  - 🔴 Vermelho: Risco de alagamento.
- **Alertas Sonoros:** Buzzer emite som em caso de alerta.
- **Display LCD I2C:** Exibe distância atual (cm ou m) e estado do rio.
- **Configurações via Serial:**
  - Unidade de Medida: cm ou m.
  - Fuso Horário: Configuração para logs corretos.
- **Data Logger:**
  - Armazena distância e timestamp na EEPROM durante alertas.
- **Gerenciamento de Logs:**
  - Visualização e exclusão de registros (requer login).
- **Animações Iniciais:** Exibição personalizada no LCD na inicialização.

---

## ⚙️ Componentes Necessários

- 1x Placa Arduino (Uno, Nano etc.)
- 1x Sensor Ultrassônico HC-SR04
- 1x Display LCD I2C (16x2 ou 20x4, endereço 0x27)
- 1x Módulo RTC DS1307 (com bateria CR2032)
- 1x Buzzer
- LEDs: 1x Verde, 1x Amarelo, 1x Vermelho
- Resistores (220Ω) para LEDs
- Fios jumper
- Protoboard (opcional)

---

## 🔌 Esquema de Conexão

### Sensor Ultrassônico (HC-SR04)
- VCC → 5V
- GND → GND
- TRIG → Pino digital 9
- ECHO → Pino digital 10

### Display LCD I2C
- VCC → 5V
- GND → GND
- SDA → A4 (Uno/Nano)
- SCL → A5 (Uno/Nano)

### Módulo RTC DS1307
- VCC → 5V
- GND → GND
- SDA → A4 (Uno/Nano)
- SCL → A5 (Uno/Nano)

### Buzzer
- Terminal positivo → Pino digital 2
- Terminal negativo → GND (com ou sem resistor)

### LEDs
- **Verde:** Ânodo → Pino 7 (via resistor 220Ω), Cátodo → GND  
- **Amarelo:** Ânodo → Pino 6 (via resistor 220Ω), Cátodo → GND  
- **Vermelho:** Ânodo → Pino 5 (via resistor 220Ω), Cátodo → GND  

---

## 📷 Visão Geral da Montagem
![image](https://github.com/user-attachments/assets/0620ca9f-61c1-43ac-9324-5ce919b4e351)
---

## 💻 Bibliotecas Necessárias

Instale via **Arduino IDE**: Sketch > Include Library > Manage Libraries...

- `LiquidCrystal_I2C` – Comunicação com o display LCD.
- `RTClib` – Comunicação com o módulo RTC.
- `EEPROM` – Leitura/escrita na memória EEPROM (padrão da IDE).
- `Wire` – Comunicação I2C (padrão da IDE).

---

## 🚀 Como Usar

1. **Monte o hardware:** Siga o esquema de conexão acima.
2. **Abra a IDE do Arduino** e cole o código fornecido.
3. **Instale as bibliotecas** necessárias.
4. **Configure a placa e a porta** correta em Tools > Board / Port.
5. **Carregue o código:** Clique em "Upload".
6. **Abra o Monitor Serial:** 9600 bauds.

---

## 🧭 Interagindo com o Menu Serial

### Menu Principal:
- `1` – Configurar Unidade de Medida
- `2` – Configurar Fuso Horário
- `3` – Acessar Data Logger
![image](https://github.com/user-attachments/assets/a949a299-0883-4992-980e-8284e06b813c)
---

### Configurações de Distância:
- `1` – Exibir em centímetros  
- `2` – Exibir em metros  
- `voltar` – Retorna ao menu principal
![image](https://github.com/user-attachments/assets/4a5c5bba-36b2-4f28-95ad-6782eddb8c04)
---

### Configurações de Fuso Horário:
- Digite um número inteiro (ex: `-3` para GMT-3)
- `voltar` – Retorna ao menu principal
![image](https://github.com/user-attachments/assets/3637557d-f04f-4377-b2fd-91ff4f3241a6)
---

### Data Logger:
- `1` – Exibir registros  
- `2` – Apagar registros (requer login)  
  - Usuário: `Admin`  
  - Senha: `1234`  
- `voltar` – Retorna ao menu principal
![image](https://github.com/user-attachments/assets/26350d9b-cfa3-4c8b-82c4-105c188b968f)
---

## ⚠️ Observações Importantes

- **Endereço do LCD:** Verifique se o endereço I2C (geralmente 0x27) está correto. Use um scanner I2C se necessário.
- **Bateria do RTC:** Instale uma CR2032 para manter a hora sem alimentação.

---
