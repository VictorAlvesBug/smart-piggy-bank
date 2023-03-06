

#include <LiquidCrystal_I2C.h>;

#define QTDE_CARACTERES_LINHA_DISPLAY   20
#define QTDE_LINHAS_DISPLAY             4 

LiquidCrystal_I2C lcd(0x27, QTDE_CARACTERES_LINHA_DISPLAY, QTDE_LINHAS_DISPLAY);

// Telas
#define TELA_MENSAGEM                    0
#define TELA_STAND_BY                    1
#define TELA_BLOQUEIO                    2
#define TELA_BLOQUEIO_ALTERAR_SENHA      3
#define TELA_NOVA_SENHA                  4
#define TELA_CONFIRMAR_NOVA_SENHA        5
#define TELA_MENU_PRINCIPAL              6
#define TELA_MENU_MEU_DINHEIRO           7
#define TELA_MENU_METAS                  8
#define TELA_MENU_META_BASE              9
#define TELA_ALTERAR_META_BASE           10
#define TELA_MENU_SUB_METAS              11
#define TELA_ALTERAR_SUB_META            12
#define TELA_DEPOSITAR_DINHEIRO          13
#define TELA_CONFIRMAR_SAQUE_DINHEIRO    14

// Acões do menu principal
#define OPCAO_MY_MONEY                   'A'
#define OPCAO_MY_GOALS                   'B'
#define OPCAO_CHANGE_PASSWORD            'C'
#define OPCAO_LOCK_SCREEN                'D'

// Acões do menu meu dinheiro
#define OPCAO_DEPOSIT_MONEY              'A'
#define OPCAO_WITHDRAW_MONEY             'B'

// Ações do menu de metas
#define OPCAO_MENU_MAIN_GOAL             'A'
#define OPCAO_MENU_SUBGOALS              'B'
#define OPCAO_BACK_TO_MENU               'C'

// Ações do menu da meta base e das sub metas
#define OPCAO_CHANGE_MAIN_GOAL           'A'
#define OPCAO_SET_OR_CHANGE_SUBGOAL      'A'
#define OPCAO_NEXT_SUBGOAL               'B'
#define OPCAO_BACK_TO_MY_GOALS           'C'

#define OPCAO_CONFIRM_WITHDRAW           'A'
#define OPCAO_CANCEL_WITHDRAW            'B'

// Identifica a tela que está sendo exibida
byte telaAtualDisplay;

// Identifica para qual tela deve ser retornado após a exibição da mensagem
byte telaRetornarDisplay = -1;

bool alterouDisplay;

// Informação a ser exibida na Tela de Mensagem
String mensagemDisplay;

long instanteAntesExibicaoMensagem = 0;

long instanteUltimaInteracao = 0;

byte offsetScrollTexto0 = 0;
byte offsetScrollTexto1 = 0;
byte offsetScrollTexto2 = 0;
byte offsetScrollTexto3 = 0;

String displayLinha0 = "";
String displayLinha1 = "";
String displayLinha2 = "";
String displayLinha3 = "";

void acenderLuzFundoDisplay(){
  lcd.backlight();
}

void apagarLuzFundoDisplay(){
  lcd.noBacklight();
}

void bloquearTelaDisplay(){
  telaAtualDisplay = TELA_STAND_BY;
  apagarLuzFundoDisplay();
  alterouDisplay = true;
}

void inicializarDisplay(){
  Wire.begin(10, 8);
  lcd.init();
  lcd.noBacklight();
  bloquearTelaDisplay();
  mensagemDisplay = F("");
}

void fecharMensagemDisplay(){
  telaAtualDisplay = telaRetornarDisplay;
  alterouDisplay = true;
}

void controleExibicaoMensagem(){
  bool estaExibindoMensagem = telaAtualDisplay == TELA_MENSAGEM;
  long duracaoExibicaoMensagem = 3000;

  if(estaExibindoMensagem){
    if(instanteAntesExibicaoMensagem + duracaoExibicaoMensagem < millis()){
      fecharMensagemDisplay();
    }
  }
  else{
    instanteAntesExibicaoMensagem = millis();
  }
}

bool excedeuTempoLimiteTelaAtiva(){
  bool telaEstaAtiva = telaAtualDisplay != TELA_STAND_BY && telaAtualDisplay != TELA_BLOQUEIO && telaAtualDisplay != TELA_MENSAGEM;
  long duracaoTelaAtivaAteAutoBloqueio = 120000;
  long tempoAviso = 10000;
  bool excedeuTempoLimite = instanteUltimaInteracao + duracaoTelaAtivaAteAutoBloqueio < millis();
  bool exibirAvisoExcederaTempoLimiteEmBreve = instanteUltimaInteracao + duracaoTelaAtivaAteAutoBloqueio < millis() + tempoAviso;

  if(telaEstaAtiva && !excedeuTempoLimite && exibirAvisoExcederaTempoLimiteEmBreve){
    telaRetornarDisplay = telaAtualDisplay;
    telaAtualDisplay = TELA_MENSAGEM;
    mensagemDisplay = F("WARNING: The screen will block automatically...");
    apagarLuzFundoDisplay();
    alterouDisplay = true;
  }

  return telaEstaAtiva && excedeuTempoLimite;
}

bool excedeuTempoLimiteTelaBloqueio(){
  bool estaExibindoTelaBloqueio = telaAtualDisplay == TELA_BLOQUEIO;
  int duracaoTelaDeBloqueioAteAutoBloqueio = 5000;
  bool excedeuTempoLimite = instanteUltimaInteracao + duracaoTelaDeBloqueioAteAutoBloqueio < millis();

  return estaExibindoTelaBloqueio && excedeuTempoLimite;
}

void controleAutoBloqueioDeTela(){
  if(excedeuTempoLimiteTelaAtiva() || excedeuTempoLimiteTelaBloqueio()){
      bloquearTelaDisplay();
  }
}

String retornarTextoLinha(byte indiceLinha, String texto) {
  String delimitador = F("\n");
  byte contador = 0;
  byte posicaoDelimitador = 0;
  String trecho;

  while ((posicaoDelimitador = texto.indexOf(delimitador)) != -1) {
    trecho = texto.substring(0, posicaoDelimitador);

    if (indiceLinha == contador) {
      return trecho;
    }
    texto = texto.substring(posicaoDelimitador + delimitador.length(), texto.length());
    if (trecho.length() > 0)
    {
      contador++;
    }
  }

  if (indiceLinha == contador) {
    return texto;
  }
  return F("");
}

String preencherTexto(String texto, byte qtdeCaracteres) {
  while (texto.length() < qtdeCaracteres) {
    texto += F(" ");
  }
  return texto;
}

String preencherTextoCentralizado(String texto, byte qtdeCaracteres) {
  bool adicionarNaDireita = true;

  // Caso o texto seja menor que o tamanho necessário, adiciona espaço até que
  // o texto tenha o tamanho correto
  while (texto.length() < qtdeCaracteres) {
    texto = adicionarNaDireita ? texto + " " : " " + texto;
    adicionarNaDireita = !adicionarNaDireita;
  }

  // Caso o texto tenha o tamanho correto, retorna o texto
  if (texto.length() == qtdeCaracteres) {
    return texto;
  }

  // Caso o texto tenha excedido o tamanho necessário, exibe o trecho que cabe
  // e adiciona reticências no final
  String trechoExibir = texto.substring(0, qtdeCaracteres - 3);
  return trechoExibir + F("...");
}

String retornarTrechoTextoOffsetScroll(String texto, byte offsetInicial){
  byte offsetFinal = offsetInicial + 20;
  return texto.substring(offsetInicial, offsetFinal);
}

void exibirNoDisplay(String trecho0, String trecho1, String trecho2, String trecho3) {
  displayLinha0 = preencherTexto(trecho0, 20);
  displayLinha1 = preencherTexto(trecho1, 20);
  displayLinha2 = preencherTexto(trecho2, 20);
  displayLinha3 = preencherTexto(trecho3, 20);

  lcd.setCursor(0, 0);
  lcd.print(retornarTrechoTextoOffsetScroll(displayLinha0, offsetScrollTexto0));
  lcd.setCursor(0, 1);
  lcd.print(retornarTrechoTextoOffsetScroll(displayLinha1, offsetScrollTexto1));
  lcd.setCursor(0, 2);
  lcd.print(retornarTrechoTextoOffsetScroll(displayLinha2, offsetScrollTexto2));
  lcd.setCursor(0, 3);
  lcd.print(retornarTrechoTextoOffsetScroll(displayLinha3, offsetScrollTexto3));
}

String repetirCaracter(char caracter, byte qtdeVezes){
  String retorno = F("");
  while(retorno.length() < qtdeVezes){
    retorno += caracter;
  }
  return retorno;
}

String quebrarLinhaTextoDisplay(String texto) {
  String retorno = F("");
  byte contadorLinhas = 0;

  while (texto.length() > QTDE_CARACTERES_LINHA_DISPLAY && contadorLinhas < QTDE_LINHAS_DISPLAY - 1) {
    byte indiceEspaco = texto.substring(0, QTDE_CARACTERES_LINHA_DISPLAY).lastIndexOf(F(" "));

    if (indiceEspaco == -1) {
      retorno += texto.substring(0, QTDE_CARACTERES_LINHA_DISPLAY) + F("\n");
      texto = texto.substring(QTDE_CARACTERES_LINHA_DISPLAY, texto.length());
      contadorLinhas++;
    }
    else {
      retorno += preencherTextoCentralizado(texto.substring(0, indiceEspaco), QTDE_CARACTERES_LINHA_DISPLAY) + F("\n");
      texto = texto.substring(indiceEspaco + 1, texto.length());
      contadorLinhas++;
    }
  }

  retorno += preencherTextoCentralizado(texto, QTDE_CARACTERES_LINHA_DISPLAY) + F("\n");
  contadorLinhas++;

  bool adicionarLinhaVaziaEmBaixo = true;

  while (contadorLinhas < QTDE_LINHAS_DISPLAY) {
    String linhaVazia = repetirCaracter(' ', QTDE_CARACTERES_LINHA_DISPLAY) + F("\n");

    if (adicionarLinhaVaziaEmBaixo) {
      retorno = retorno + linhaVazia;
    }
    else {
      retorno = linhaVazia + retorno;
    }
    contadorLinhas++;
    adicionarLinhaVaziaEmBaixo = !adicionarLinhaVaziaEmBaixo;
  }

  return retorno;
}