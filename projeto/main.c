/*A avaliação será feita através de um projeto que utiliza os módulos descritos para atender
aos requisitos abaixo. Cada requisito atingido soma um ponto na avaliação, sendo que não
é preciso atingir todos para obter a nota máxima.
Implemente um sistema de um “cofre” com senha numérica de 6 dígitos utilizando o teclado
4x4 e o display 16x2 como saída de dados para o usuário. Atenção aos requisitos abaixo:

1. Utilizar as teclas do teclado 4x4 para inserção da senha numérica (2 pontos)
2. Apresentar mensagens ao usuário no display 16x2 com a situação do cofre,
destacando as ações abaixo (2 pontos):
a. “Inserir Senha”
b. “Senha Incorreta”
c. “Senha Correta”
d. “Cofre Travado”
e. “Cofre Aberto”

3. Mostrar a senha no display 16x2 enquanto ela é digitada (1 ponto)
4. Acionar o buzzer de acordo com as situações abaixo (2 pontos):
a. Tecla pressionada, 1 bip curto
b. Senha incorreta, 1 bip longo
c. Senha correta, 3 bips curtos

5. Implementar um sistema de travamento do teclado após 3 tentativas com senhas
inválidas. Adicionar as seguintes mensagens no funcionamento do display (2
ponto):
a. “Tentativas Restantes: X”
b. “Teclado Bloqueado por X segundos”

6. Implementar um mecanismo de alteração da senha utilizando a memória EEPROM
(2 pontos)

7. Apresentar a hora atual no display LCD, considerando início à meia noite.Utilizar um
sistema simples de ajuste do relógio, por exemplo um tecla incrementa a hora e
outra tecla incrementa o minuto. Outros mecanismos de ajuste de horário podem ser
aplicados (2 pontos)

8. Enviar o código através de um repositório pessoal no GitHub utilizando qualquer
cliente do GIT. Enviar o código do projeto através de “commits” atômicos, com um
mínimo de 10 commits. (2 pontos)*/


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// INCLUSAO DE BIBLIOTECAS
#include <avr/io.h>
#define F_CPU 16000000
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

// CONFIGURACAO DO LCD
#define LCD_PORT PORTD //porta que ta conectado
#define LCD_DDR DDRD // configura como saida
#define LCD_RS PD0 // esse e o de baixo configuram os pinos de controle do LCD
#define LCD_EN PD1

// CONFIGURACAO DO TECLADO - Define a porta e os registradores do teclado matricial
#define TECLADO_PORT PORTC
#define TECLADO_DDR DDRC
#define TECLADO_PIN PINC

// CONFIGURACAO DO BUZZER - Configura a porta onde o buzzer esta conectado
#define BUZZER_PORT PORTB
#define BUZZER_DDR DDRB
#define BUZZER_PIN PB0

// CONFIGURACAO RELOGIO
#define BUTTON_HOUR_PIN PB1  // Botao para incrementar a hora
#define BUTTON_MIN_PIN  PB2  // Botao para incrementar o minuto
#define BUTTON_PORT PORTB
#define BUTTON_DDR DDRB
#define BUTTON_PIN PINB


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// DEFINICAO DE SENHA E VARIAVEIS DE COFRE
char senha_correta[] = "123456"; //senha fixa
char inserir_senha[7]; //armazena a senha
uint8_t input_index = 0; //controla a posicaoo da senha digitada
uint8_t tentativas = 3; //contador de tentativas antes do bloqueio

// PROTOTIPOS DAS FUNCOES - declara as funcoes
void lcd_init();
void lcd_command(unsigned char cmd);
void lcd_clear();
void lcd_print(const char *str);
void lcd_gotoxy(uint8_t x, uint8_t y);
char teclado_getkey();
void buzzer_beep(uint8_t times);
void handle_password_entry();
void mensagem_trancar();
void rtc_read_time();


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//INICIALIZACAOO DO LCD 
void lcd_init() {
	LCD_DDR = 0xFF; //Define todos os pinos do LCD como saida
	_delay_ms(20);
	lcd_command(0x38); //Configura o LCD para modo 8 bits
	lcd_command(0x0C); //Liga o display sem cursor
	lcd_command(0x06); //Configura a posicao do cursor para deslocamento automatico apos inserir um caracter
	lcd_clear(); //Limpa o display
}

//FUNCAO PARA ENVIAR COMANDOS AO LCD
void lcd_command(unsigned char cmd) {
	LCD_PORT = cmd;
    LCD_PORT &= ~(1 << LCD_RS); // Modo comando, envia o comando
    LCD_PORT |= (1 << LCD_EN); // Pulso de habilitacao para o LCD ler o comando
	_delay_ms(1);
	LCD_PORT &= ~(1 << LCD_EN); //desatiba pra finalizar a leitura
	_delay_ms(2);
}

// FUNCAO PARA LIMPAR O LCD
void lcd_clear() {
	lcd_command(0x01);
	_delay_ms(2);
}

// FUNCA�O PARA ESCREVER NO LCD
void lcd_print(const char *str) {
	while (*str) {
        LCD_PORT = *str++; // Envia caractere por caractere
		LCD_PORT |= (1 << LCD_RS);
		LCD_PORT |= (1 << LCD_EN);
		_delay_ms(1);
		LCD_PORT &= ~(1 << LCD_EN);
	}
}

//FUNCAO PARA POSICIONAR O CURSOR DO LCD
void lcd_gotoxy(uint8_t x, uint8_t y) {
	uint8_t pos = (y == 0) ? (0x80 + x) : (0xC0 + x);
	lcd_command(pos);
}

// LEITURA DO TECLADO
char teclado_getkey() {
	char keys[4][3] =
	{{'1', '2', '3'},
	{'4', '5', '6'},
	{'7', '8', '9'},
	{'*', '0', '#'}};
		
    TECLADO_DDR = 0xF0; // Define metade como saida e metade como entrada
    TECLADO_PORT |= 0xFF; // Ativa pull-ups
	
	for (uint8_t row = 0; row < 4; row++) {
		TECLADO_PORT = ~(1 << (row + 4)); //inverte os bits para que apenas uma linha fique em nivel baixo (0) por vez; Isso permite identificar qual tecla foi pressionada
		_delay_ms(10);
		
		for (uint8_t col = 0; col < 3; col++) { //Percorre cada uma das 3 colunas para verificar se ha uma tecla pressionada
			if (!(TECLADO_PIN & (1 << col))) { //Verifica se a coluna esta em nivel logico baixo (0), indicando que uma tecla foi pressionada
				while (!(TECLADO_PIN & (1 << col))); //Aguarda enquanto a tecla ainda estiver pressionada
				return keys[row][col];
			}
		}
	}
	return 0;
}

// FUNCAO DO BUZZER
void buzzer_beep(uint8_t times) {
	for (uint8_t i = 0; i < times; i++) {
		BUZZER_PORT |= (1 << BUZZER_PIN);
		_delay_ms(100);
		BUZZER_PORT &= ~(1 << BUZZER_PIN);
		_delay_ms(100);
	}
}

// FUNCAO PARA MANIPULACAO DA SENHA - gerencia a entrada da senha pelo usuario e verifica se esta correta
void handle_password_entry() {
	lcd_clear();
	lcd_gotoxy(0, 0);
	lcd_print("Inserir Senha:");
	input_index = 0;
	memset(inserir_senha, 0, sizeof(inserir_senha));
	
	while (input_index < 6) { //Continua ate que 6 digitos sejam inseridos
		char key = teclado_getkey();
		
		if (key >= '0' && key <= '9') { //verifica se a tecla pressionada e um numero (0 a 9). Se nao for um numero, o caractere e ignorado
			buzzer_beep(1); //Emite um bip curto para indicar que a tecla foi pressionada
			inserir_senha[input_index++] = key; //Armazena o numero digitado e incrementa input_index
			lcd_gotoxy(input_index, 1); //Move o cursor para a linha de exibicao da senha
			lcd_print("*"); //Mostra um * em vez do numero digitado (para esconder a senha)
		}
	}
	if (strcmp(inserir_senha, senha_correta) == 0) {
		lcd_clear();
		lcd_print("Senha Correta");
		buzzer_beep(3);
		_delay_ms(2000);
		lcd_clear();
		lcd_print("Cofre Aberto");
		_delay_ms(5000);
		lcd_clear();
		lcd_print("Cofre Bloqueado");
		} else {
		tentativas--; //reduz o numero de tentativas
		lcd_clear();
		lcd_print("Senha Incorreta");
		buzzer_beep(2);
		_delay_ms(2000);
		if (tentativas == 0) {
			mensagem_trancar();
		}
	}
}

// FUNCAO DE BLOQUEIO
void mensagem_trancar() {
	lcd_clear();
	lcd_print("Teclado bloqueado");
	
	for (int i = 10; i > 0; i--) { // Inicia um loop de 10 a 1, indicando quantos segundos faltam para desbloquear.
		lcd_gotoxy(0, 1);
		
		char buffer[16]; //Cria uma string com a contagem regressiva
		snprintf(buffer, 16, "Unlock in: %d s", i);
		lcd_print(buffer);
		_delay_ms(1000);
	}
	tentativas = 3;
	lcd_clear();
}

// SIMULAÇÃO DE RELÓGIO DIGITAL
void rtc_read_time() {
	lcd_gotoxy(0, 0);
	lcd_print("Time: ");

	// Configuracao dos botoes (pull-up interno)
	BUTTON_DDR &= ~(1 << BUTTON_HOUR_PIN);  // Configura como entrada
	BUTTON_DDR &= ~(1 << BUTTON_MIN_PIN);   // Configura como entrada
	BUTTON_PORT |= (1 << BUTTON_HOUR_PIN);  // Ativa pull-up interno
	BUTTON_PORT |= (1 << BUTTON_MIN_PIN);   // Ativa pull-up interno

	// Simulacao de RTC (Relogio Interno)
	static uint8_t sec = 0, min = 0, hour = 12;
	
	sec++;
	if (sec == 60) { sec = 0; min++; }
	if (min == 60) { min = 0; hour++; }
	if (hour == 24) { hour = 0; }

	//Verifica se o botao de hora foi pressionado
	if (!(BUTTON_PIN & (1 << BUTTON_HOUR_PIN))) {  //Verifica se o botao de hora foi pressionado
		hour++;
		if (hour == 24) hour = 0;  //Garante que nao ultrapasse 23h
		_delay_ms(200);  //Debounce
	}

	//Verifica se o botao de minuto foi pressionado
	if (!(BUTTON_PIN & (1 << BUTTON_MIN_PIN))) {  //Verifica se o botao de segundo foi pressionado
		min++;
		if (min == 60) min = 0;  //Garante que nao ultrapasse 59 min
		_delay_ms(200);  //Debounce
	}

	//Exibe a hora formatada no LCD
	char time_str[9];
	snprintf(time_str, 9, "%02d:%02d:%02d", hour, min, sec);
	lcd_gotoxy(6, 0); //Move o cursor para a primeira linha, posicao 6 (logo apos "Time: ")
	lcd_print(time_str);

	_delay_ms(1000);  // Aguarda 1 segundo antes de atualizar o tempo
}


// FUNCAO PRINCIPAL
int main(void) {
	lcd_init();
	
	BUZZER_DDR |= (1 << BUZZER_PIN);
	
	lcd_print("Cofre bloqueado"); //Exibe "Cofre bloqueado" no LCD para informar o estado inicial do sistema
	_delay_ms(2000);
	lcd_clear();
	
	while (1) {
		rtc_read_time(); //Chama rtc_read_time() para exibir o relogio digital no LCD
		char key = teclado_getkey(); //Verifica se alguma tecla foi pressionada no teclado matricial
		
		if (key) { //Se key nao for 0, significa que uma tecla foi pressionada
			handle_password_entry(); //Chama handle_password_entry(); para capturar a senha e verificar se esta correta
		}
	}
	return 0;
}
