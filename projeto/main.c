
// INCLUSAO DE BIBLIOTECAS
#include <avr/io.h>
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

// CONFIGURACAO DO BUZZER - Configura a porta onde o buzzer est� conectado
#define BUZZER_PORT PORTB
#define BUZZER_DDR DDRB
#define BUZZER_PIN PB0

// CONFIGURACAO RELOGIO
#define BUTTON_HOUR_PIN PB1  // Bot�o para incrementar a hora
#define BUTTON_MIN_PIN  PB2  // Bot�o para incrementar o minuto
#define BUTTON_PORT PORTB
#define BUTTON_DDR DDRB
#define BUTTON_PIN PINB

//////////////////////////////////////////////////////////////////////////////////////////

// DEFINICAO DE SENHA E VARIAVEIS DE COFRE
char senha_correta[] = "123456"; //senha fixa
char inserir_senha[7]; //armazena a senha
uint8_t input_index = 0; //controla a posi��o da senha digitada
uint8_t tentativas = 3; //contador de tentativas antes do bloqueio

// PROT�TIPOS DAS FUN��ES - declara as fun��es
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

//////////////////////////////////////////////////////////////////////////////////////////


//INICIALIZACAOO DO LCD 
void lcd_init() {
	LCD_DDR = 0xFF; //Define todos os pinos do LCD como sa�da
	_delay_ms(20);
	lcd_command(0x38); //Configura o LCD para modo 8 bits
	lcd_command(0x0C); //Liga o display sem cursor
	lcd_command(0x06); //Configura a posi��o do cursor para deslocamento autom�tico ap�s inserir um caracter
	lcd_clear(); //Limpa o display
}

//FUN��O PARA ENVIAR COMANDOS AO LCD
void lcd_command(unsigned char cmd) {
	LCD_PORT = cmd;
    LCD_PORT &= ~(1 << LCD_RS); // Modo comando, envia o comando
    LCD_PORT |= (1 << LCD_EN); // Pulso de habilita��o para o LCD ler o comando
	_delay_ms(1);
	LCD_PORT &= ~(1 << LCD_EN); //desatiba pra finalizar a leitura
	_delay_ms(2);
}

// FUN��O PARA LIMPAR O LCD
void lcd_clear() {
	lcd_command(0x01);
	_delay_ms(2);
}

// FUN��O PARA ESCREVER NO LCD
void lcd_print(const char *str) {
	while (*str) {
        LCD_PORT = *str++; // Envia caractere por caractere
		LCD_PORT |= (1 << LCD_RS);
		LCD_PORT |= (1 << LCD_EN);
		_delay_ms(1);
		LCD_PORT &= ~(1 << LCD_EN);
	}
}

//FUN��O PARA POSICIONAR O CURSOR DO LCD
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
		
    TECLADO_DDR = 0xF0; // Define metade como sa�da e metade como entrada
    TECLADO_PORT |= 0xFF; // Ativa pull-ups
	
	for (uint8_t row = 0; row < 4; row++) {
		TECLADO_PORT = ~(1 << (row + 4)); //inverte os bits para que apenas uma linha fique em n�vel baixo (0) por vez; Isso permite identificar qual tecla foi pressionada
		_delay_ms(10);
		
		for (uint8_t col = 0; col < 3; col++) { //Percorre cada uma das 3 colunas para verificar se h� uma tecla pressionada
			if (!(TECLADO_PIN & (1 << col))) { //Verifica se a coluna est� em n�vel l�gico baixo (0), indicando que uma tecla foi pressionada
				while (!(TECLADO_PIN & (1 << col))); //Aguarda enquanto a tecla ainda estiver pressionada
				return keys[row][col];
			}
		}
	}
	return 0;
}

// FUN��O DO BUZZER
void buzzer_beep(uint8_t times) {
	for (uint8_t i = 0; i < times; i++) {
		BUZZER_PORT |= (1 << BUZZER_PIN);
		_delay_ms(100);
		BUZZER_PORT &= ~(1 << BUZZER_PIN);
		_delay_ms(100);
	}
}

// FUN��O PARA MANIPULA��O DA SENHA - gerencia a entrada da senha pelo usu�rio e verifica se est� correta
void handle_password_entry() {
	lcd_clear();
	lcd_gotoxy(0, 0);
	lcd_print("Inserir Senha:");
	input_index = 0;
	memset(inserir_senha, 0, sizeof(inserir_senha));
	
	while (input_index < 6) { //Continua at� que 6 d�gitos sejam digitados
		char key = teclado_getkey();
		
		if (key >= '0' && key <= '9') { //verifica se a tecla pressionada � um n�mero (0 a 9). Se n�o for um n�mero, o caractere � ignorado
			buzzer_beep(1); //Emite um bip curto para indicar que a tecla foi pressionada
			inserir_senha[input_index++] = key; //Armazena o n�mero digitado e incrementa input_index
			lcd_gotoxy(input_index, 1); //Move o cursor para a linha de exibi��o da senha
			lcd_print("*"); //Mostra um * em vez do n�mero digitado (para esconder a senha)
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

// FUN��O DE BLOQUEIO
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
	attempts = 3;
	lcd_clear();
}

// Simula��o do Rel�gio digital
void rtc_read_time() {
	lcd_gotoxy(0, 0);
	lcd_print("Time: ");

	// Configura��o dos bot�es (pull-up interno)
	BUTTON_DDR &= ~(1 << BUTTON_HOUR_PIN);  // Configura como entrada
	BUTTON_DDR &= ~(1 << BUTTON_MIN_PIN);   // Configura como entrada
	BUTTON_PORT |= (1 << BUTTON_HOUR_PIN);  // Ativa pull-up interno
	BUTTON_PORT |= (1 << BUTTON_MIN_PIN);   // Ativa pull-up interno

	// Simula��o de RTC (Rel�gio Interno)
	static uint8_t sec = 0, min = 0, hour = 12;
	
	sec++;
	if (sec == 60) { sec = 0; min++; }
	if (min == 60) { min = 0; hour++; }
	if (hour == 24) { hour = 0; }

	// Verifica se o bot�o de hora foi pressionado
	if (!(BUTTON_PIN & (1 << BUTTON_HOUR_PIN))) {  // Verifica se o bot�o de hora foi pressionado
		hour++;
		if (hour == 24) hour = 0;  // Garante que n�o ultrapasse 23h
		_delay_ms(200);  //Debounce
	}

	// Verifica se o bot�o de minuto foi pressionado
	if (!(BUTTON_PIN & (1 << BUTTON_MIN_PIN))) {  // Verifica se o bot�o de segundo foi pressionado
		min++;
		if (min == 60) min = 0;  // Garante que n�o ultrapasse 59 min
		_delay_ms(200);  //Debounce
	}

	// Exibe a hora formatada no LCD
	char time_str[9];
	snprintf(time_str, 9, "%02d:%02d:%02d", hour, min, sec);
	lcd_gotoxy(6, 0); //Move o cursor para a primeira linha, posi��o 6 (logo ap�s "Time: ")
	lcd_print(time_str);

	_delay_ms(1000);  // Aguarda 1 segundo antes de atualizar o tempo
}


// FUN��O PRINCIPAL
int main(void) {
	lcd_init();
	
	BUZZER_DDR |= (1 << BUZZER_PIN);
	
	lcd_print("Cofre bloqueado"); //Exibe "Cofre bloqueado" no LCD para informar o estado inicial do sistema
	_delay_ms(2000);
	lcd_clear();
	
	while (1) {
		rtc_read_time(); //Chama rtc_read_time() para exibir o rel�gio digital no LCD
		char key = teclado_getkey(); //Verifica se alguma tecla foi pressionada no teclado matricial
		if (key) { //Se key n�o for 0, significa que uma tecla foi pressionada
			handle_password_entry(); //Chama handle_password_entry(); para capturar a senha e verificar se est� correta
		}
	}
}
