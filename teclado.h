
#include <Keypad.h>

// Quantidade de linhas e colunas do teclado
#define qtdeLinhas   4
#define qtdeColunas  4

// Construção da matriz de teclaes
char matriz_teclas[qtdeLinhas][qtdeColunas] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'<', '0', '>', 'D'}
};

// Pinos utilizados pelas linhas e colunas do teclado
byte pinosLinhas[qtdeLinhas] = {20, 21, 7, 6};
byte pinosColunas[qtdeColunas] = {5, 4, 3, 2};

// Inicialização do teclado
Keypad teclado = Keypad(
  makeKeymap(matriz_teclas),
  pinosLinhas,
  pinosColunas,
  qtdeLinhas,
  qtdeColunas
);

bool ehNumero(char tecla) {
  return tecla == '0'
         || tecla == '1'
         || tecla == '2'
         || tecla == '3'
         || tecla == '4'
         || tecla == '5'
         || tecla == '6'
         || tecla == '7'
         || tecla == '8'
         || tecla == '9';
}

bool ehConfirmar(char tecla) {
  return tecla == '>';
}

bool ehApagar(char tecla) {
  return tecla == '<';
}

bool ehCancelar(char tecla){
  return tecla == 'C';
}

bool ehDeletar(char tecla){
  return tecla == 'D';
}