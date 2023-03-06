/*
  Cenários Testar:

  - Confirmar senha vazia
  - Digitar senha até limite de 16 caracteres
  - Confirmar senha errada
  - Digitar antes de voltar da mensagem de erro
  - Confirmar senha certa
  - Digitar valor 210 e verificar se ele mantem dígitos ou muda para 209
  - Verificar se subgoal decimal avisa quando é atingida
*/

#include "teclado.h";
#include "display.h";
#include "utils-mqtt.h";


#define QTDE_MAXIMA_SUB_METAS          5

struct Meta {
  float valor;
  bool foiAtingida;
};

// Senha da tela de bloqueio
String senhaCorreta;
String senhaDigitada;
String senhaDigitadaNova;

// Metas
Meta metaBase = { 500, false };

Meta subMetas[QTDE_MAXIMA_SUB_METAS] = {
  { 0, false},
  { 0, false},
  { 0, false},
  { 0, false},
  { 0, false}
};

float valorLimiteMinimo = 0.01;
float valorLimiteMaximo = 999.99;

byte indiceMenuSubMetaAtual = 0;

float valorDigitado = 0;

// Valor Guardado
float valorGuardado = 0;

long instanteCursorAlternou = 0;
bool cursorAtivo = true;

String topicoMQTT = "smart_piggy_bank/message";
String mensagemPublicarMQTT = "";

void publicarAtualizacaoMetasMQTT() {
  mensagemPublicarMQTT = "";

  mensagemPublicarMQTT += F("Goals updated: Your main goal is ");
  mensagemPublicarMQTT += aplicarMascaraDinheiro(metaBase.valor);

  byte qtdeSubMetasComValor = 0;

  for (byte indice = 0; indice < QTDE_MAXIMA_SUB_METAS; indice++) {
    if (subMetas[indice].valor > 0) {
      qtdeSubMetasComValor++;
    }
  }

  if (qtdeSubMetasComValor == 0) {
    mensagemPublicarMQTT += F(" and you do not have subgoals.");
  }
  else if (qtdeSubMetasComValor == 1) {
    mensagemPublicarMQTT += F(" and your subgoal is ");
    for (byte indice = 0; indice < QTDE_MAXIMA_SUB_METAS; indice++) {
      if (subMetas[indice].valor > 0) {
        mensagemPublicarMQTT += aplicarMascaraDinheiro(subMetas[indice].valor);
      }
    }

    mensagemPublicarMQTT += F(".");
  }
  else {
    byte indiceSubMetaComValor = 0;
    mensagemPublicarMQTT += F(" and your subgoals are ");
    for (byte indice = 0; indice < QTDE_MAXIMA_SUB_METAS; indice++) {
      if (subMetas[indice].valor > 0) {
        mensagemPublicarMQTT += aplicarMascaraDinheiro(subMetas[indice].valor);
        if (indiceSubMetaComValor < qtdeSubMetasComValor - 2) {
          mensagemPublicarMQTT += F("; ");
        }
        else if (indiceSubMetaComValor == qtdeSubMetasComValor - 2) {
          mensagemPublicarMQTT += F(" and ");
        }
        indiceSubMetaComValor++;
      }
    }

    mensagemPublicarMQTT += F(".");
  }

  publicarNoTopicoMQTT(topicoMQTT, mensagemPublicarMQTT);
}

void setup() {
  // Inicialização Serial
  Serial.begin(115200);

  // Inicialização Display LCD, via adaptador I2C
  inicializarDisplay();

  senhaCorreta = F("0000");
  senhaDigitada = F("");
  senhaDigitadaNova = F("");

  inicializarMQTT();
}

void loop() {
  // Trata o click das teclas do teclado
  computarTeclaPressionada();

  //if(instanteCursorAlternou + 500 < millis()){
  //  instanteCursorAlternou = millis();
  //  cursorAtivo = !cursorAtivo;
  //  alterouDisplay = true;
  //}

  controleExibicaoMensagem();

  controleAutoBloqueioDeTela();

  atualizarMQTT();

  // Altera tela do display, caso seja necessário
  if (alterouDisplay) {
    alterouDisplay = false;
    atualizarDisplay();
  }
}

void atualizarDisplay() {
  String linhaBranca = repetirCaracter(' ', 20);
  String linhaPreta = repetirCaracter('#', 20);

  String textoDisplay = F("");

  String textoLinha0 = F("");
  String textoLinha1 = F("");
  String textoLinha2 = F("");
  String textoLinha3 = F("");

  String mensagemOriginal = F("");

  switch (telaAtualDisplay) {
    case TELA_MENSAGEM:
      textoDisplay = quebrarLinhaTextoDisplay(mensagemDisplay);

      textoLinha0 = retornarTextoLinha(0, textoDisplay);
      textoLinha1 = retornarTextoLinha(1, textoDisplay);
      textoLinha2 = retornarTextoLinha(2, textoDisplay);
      textoLinha3 = retornarTextoLinha(3, textoDisplay);

      exibirNoDisplay(
        textoLinha0,
        textoLinha1,
        textoLinha2,
        textoLinha3
      );
      break;

    case TELA_STAND_BY:
      // Exibe o nome do dispositivo com fundo de hashtag (#)
      exibirNoDisplay(
        linhaPreta,
        F("# SMART PIGGY BANK #"),
        linhaPreta,
        linhaPreta
      );
      break;

    case TELA_BLOQUEIO:
      // Exibe a representação da senha que está sendo digitada
      exibirNoDisplay(
        F("Password:"),
        ocultarSenha(senhaDigitada) + cursor(),
        linhaBranca,
        F("C: Cancel")
      );
      break;

    case TELA_BLOQUEIO_ALTERAR_SENHA:
      // Exibe a representação da senha que está sendo digitada
      exibirNoDisplay(
        F("Current password:"),
        ocultarSenha(senhaDigitada) + cursor(),
        linhaBranca,
        F("C: Cancel")
      );
      break;

    case TELA_NOVA_SENHA:
      // Exibe a representação da senha que está sendo digitada
      exibirNoDisplay(
        F("New password:"),
        ocultarSenha(senhaDigitada) + cursor(),
        linhaBranca,
        F("C: Cancel")
      );
      break;

    case TELA_CONFIRMAR_NOVA_SENHA:
      // Exibe a representação da senha que está sendo digitada
      exibirNoDisplay(
        F("Confirm password:"),
        ocultarSenha(senhaDigitada) + cursor(),
        linhaBranca,
        F("C: Cancel")
      );
      break;

    case TELA_MENU_PRINCIPAL:
      exibirNoDisplay(
        F("A: My money"),
        F("B: My goals"),
        F("C: Change password"),
        F("D: Lock screen")
      );
      break;

    case TELA_MENU_MEU_DINHEIRO:
      exibirNoDisplay(
        "You have " + aplicarMascaraDinheiro(valorGuardado),
        F("A: Deposit money"),
        F("B: Withdraw money"),
        F("C: Back to menu")
      );
      break;

    case TELA_MENU_METAS:
      exibirNoDisplay(
        F("A: My main goal"),
        F("B: My subgoals"),
        F("C: Back to menu"),
        linhaBranca
      );
      break;

    case TELA_MENU_META_BASE:
      exibirNoDisplay(
        F("Your main goal is:"),
        aplicarMascaraDinheiro(metaBase.valor),
        F("A: Change it"),
        F("C: Back to menu")
      );
      break;

    case TELA_ALTERAR_META_BASE:
      exibirNoDisplay(
        F("Edit the main goal:"),
        aplicarMascaraDinheiro(valorDigitado) + cursor(),
        linhaBranca,
        F("C: Cancel")
      );
      break;

    case TELA_MENU_SUB_METAS:
      if (subMetas[indiceMenuSubMetaAtual].valor == 0) {
        exibirNoDisplay(
          "Subgoal " + String(indiceMenuSubMetaAtual + 1) + ": (empty)",
          F("A: Set it"),
          F("B: Next subgoal"),
          F("C: Back to menu")
        );
      }
      else {
        exibirNoDisplay(
          "Subgoal " + String(indiceMenuSubMetaAtual + 1) + ": " + aplicarMascaraDinheiro(subMetas[indiceMenuSubMetaAtual].valor),
          F("A: Change it"),
          F("B: Next subgoal"),
          F("C: Back to menu")
        );
      }
      break;

    case TELA_ALTERAR_SUB_META:
      exibirNoDisplay(
        F("Edit the subgoal:"),
        aplicarMascaraDinheiro(valorDigitado) + cursor(),
        F("C: Cancel"),
        F("D: Delete subgoal")
      );
      break;

    case TELA_DEPOSITAR_DINHEIRO:
      exibirNoDisplay(
        F("Type the amount you"),
        F("want to deposit:"),
        aplicarMascaraDinheiro(valorDigitado) + cursor(),
        F("C: Cancel")
      );
      break;

    case TELA_CONFIRMAR_SAQUE_DINHEIRO:
      exibirNoDisplay(
        F("You are about to"),
        "withdraw " + aplicarMascaraDinheiro(valorGuardado),
        F("A: Confirm withdraw"),
        F("B: Cancel")
      );
      break;
  }
}

String cursor() {
  return cursorAtivo ? F("_") : F(" ");
}

String ocultarSenha(String senha) {
  String retorno = F("");

  for (byte indiceCaracter = 0; indiceCaracter < senha.length(); indiceCaracter++) {
    retorno += F("*");
  }

  return retorno;
}

void computarTeclaPressionada() {
  // Recupera a tecla pressionada
  char tecla = teclado.getKey();

  // Caso a tecla seja inválida (nenhuma tecla foi pressionada)
  if (!tecla) {
    // Sai do método
    return;
  }

  instanteUltimaInteracao = millis();
  acenderLuzFundoDisplay();

  // Computa a tecla pressionada, conforme a tela atual
  switch (telaAtualDisplay) {
    case TELA_MENSAGEM:
      fecharMensagemDisplay();
      break;

    case TELA_STAND_BY:
      telaAtualDisplay = TELA_BLOQUEIO;
      senhaDigitada = F("");
      computarSenhaDigitada(tecla);
      break;

    case TELA_BLOQUEIO:
    case TELA_BLOQUEIO_ALTERAR_SENHA:
    case TELA_NOVA_SENHA:
    case TELA_CONFIRMAR_NOVA_SENHA:
      computarSenhaDigitada(tecla);
      break;

    case TELA_MENU_PRINCIPAL:
      computarAcaoMenuPrincipal(tecla);
      break;

    case TELA_MENU_MEU_DINHEIRO:
      computarAcaoMenuMeuDinheiro(tecla);
      break;

    case TELA_MENU_METAS:
      computarAcaoMenuMetas(tecla);
      break;

    case TELA_MENU_META_BASE:
      computarAcaoMenuMetaBase(tecla);
      break;

    case TELA_ALTERAR_META_BASE:
      computarMetaBaseDigitada(tecla);
      break;

    case TELA_MENU_SUB_METAS:
      computarAcaoMenuSubMetas(tecla);
      break;

    case TELA_ALTERAR_SUB_META:
      computarSubMetaDigitada(tecla);
      break;

    case TELA_DEPOSITAR_DINHEIRO:
      computarValorDepositoDigitado(tecla);
      break;

    case TELA_CONFIRMAR_SAQUE_DINHEIRO:
      computarAcaoConfirmarSaqueDinheiro(tecla);
      break;
  }

  alterouDisplay = true;
}

void computarSenhaDigitada(char tecla) {
  if (ehNumero(tecla)) {
    // Se for um número e senha não excedeu 16 caracteres, adiciona no final da
    // senha digitada
    if (senhaDigitada.length() < 16) {
      senhaDigitada += tecla;
    }
    else {
      telaRetornarDisplay = telaAtualDisplay;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = F("Password must be 4-16 characters");
    }
  }
  else if (ehApagar(tecla)) {
    // Se for a tecla de apagar, remove o último número da senha digitada
    senhaDigitada = senhaDigitada.substring(0, senhaDigitada.length() - 1);
  }
  else if (ehConfirmar(tecla)) {
    // Se for a tecla de confirmar e a senha tiver entre 4 e 16 caracteres,
    // confere se a senha está correta
    computarTeclaConfirmarSenha();
  }
  else if (ehCancelar(tecla)) {
    computarTeclaCancelarSenha();
  }
}

String retornarMensagemSenha(bool ehNovaSenha) {
  if (!ehNovaSenha && senhaDigitada != senhaCorreta) {
    return F("Incorrect password");
  }

  if (senhaDigitada.length() == 0) {
    return F("Type the password");
  }

  if (senhaDigitada.length() < 4 || senhaDigitada.length() > 16) {
    return F("Password must be 4-16 characters");
  }

  return F("");
}

void computarTeclaConfirmarSenha() {
  bool ehNovaSenha;

  switch (telaAtualDisplay) {
    case TELA_BLOQUEIO:
      ehNovaSenha = false;
      mensagemDisplay = retornarMensagemSenha(ehNovaSenha);

      if (mensagemDisplay.length() == 0) {
        telaAtualDisplay = TELA_MENU_PRINCIPAL;
      }
      else {
        telaRetornarDisplay = telaAtualDisplay;
        telaAtualDisplay = TELA_MENSAGEM;
      }

      senhaDigitada = F("");
      senhaDigitadaNova = F("");
      break;

    case TELA_BLOQUEIO_ALTERAR_SENHA:
      ehNovaSenha = false;
      mensagemDisplay = retornarMensagemSenha(ehNovaSenha);

      if (mensagemDisplay.length() == 0) {
        telaAtualDisplay = TELA_NOVA_SENHA;
      }
      else {
        telaRetornarDisplay = telaAtualDisplay;
        telaAtualDisplay = TELA_MENSAGEM;
      }

      senhaDigitada = F("");
      senhaDigitadaNova = F("");
      break;

    case TELA_NOVA_SENHA:
      ehNovaSenha = true;
      mensagemDisplay = retornarMensagemSenha(ehNovaSenha);

      if (mensagemDisplay.length() == 0) {
        telaAtualDisplay = TELA_CONFIRMAR_NOVA_SENHA;
        senhaDigitadaNova = senhaDigitada;
      }
      else {
        telaRetornarDisplay = telaAtualDisplay;
        telaAtualDisplay = TELA_MENSAGEM;
        senhaDigitadaNova = F("");
      }

      senhaDigitada = F("");
      break;

    case TELA_CONFIRMAR_NOVA_SENHA:
      ehNovaSenha = true;

      if (senhaDigitada == senhaDigitadaNova) {
        telaRetornarDisplay = TELA_MENU_PRINCIPAL;
        telaAtualDisplay = TELA_MENSAGEM;
        mensagemDisplay = F("Password changed successfully!");
        senhaCorreta = senhaDigitada;
      }
      else {
        telaRetornarDisplay = TELA_NOVA_SENHA;
        telaAtualDisplay = TELA_MENSAGEM;
        mensagemDisplay = F("Both new passwords must match");
      }

      senhaDigitada = F("");
      senhaDigitadaNova = F("");
      break;
  }
}

void computarTeclaCancelarSenha() {
  switch (telaAtualDisplay) {
    case TELA_BLOQUEIO:
      bloquearTelaDisplay();
      break;

    case TELA_BLOQUEIO_ALTERAR_SENHA:
      telaAtualDisplay = TELA_MENU_PRINCIPAL;
      senhaDigitada = F("");
      senhaDigitadaNova = F("");
      break;

    case TELA_NOVA_SENHA:
      telaAtualDisplay = TELA_MENU_PRINCIPAL;
      senhaDigitada = F("");
      senhaDigitadaNova = F("");
      break;

    case TELA_CONFIRMAR_NOVA_SENHA:
      telaAtualDisplay = TELA_MENU_PRINCIPAL;
      senhaDigitada = F("");
      senhaDigitadaNova = F("");
      break;
  }
}

void computarAcaoMenuPrincipal(char tecla) {
  switch (tecla) {
    case OPCAO_MY_MONEY:
      telaAtualDisplay = TELA_MENU_MEU_DINHEIRO;
      break;

    case OPCAO_MY_GOALS:
      telaAtualDisplay = TELA_MENU_METAS;
      break;

    case OPCAO_CHANGE_PASSWORD:
      telaAtualDisplay = TELA_BLOQUEIO_ALTERAR_SENHA;
      senhaDigitada = F("");
      break;

    case OPCAO_LOCK_SCREEN:
      bloquearTelaDisplay();
      break;
  }
}

void computarAcaoMenuMeuDinheiro(char tecla) {
  switch (tecla) {
    case OPCAO_DEPOSIT_MONEY:
      telaAtualDisplay = TELA_DEPOSITAR_DINHEIRO;
      valorDigitado = 0;
      break;

    case OPCAO_WITHDRAW_MONEY:
      if (valorGuardado == 0) {
        telaRetornarDisplay = telaAtualDisplay;
        telaAtualDisplay = TELA_MENSAGEM;
        mensagemDisplay = F("You do not have money to withdraw");
      }
      else {
        telaAtualDisplay = TELA_CONFIRMAR_SAQUE_DINHEIRO;
      }
      break;

    case OPCAO_BACK_TO_MENU:
      telaAtualDisplay = TELA_MENU_PRINCIPAL;
      break;
  }
}

void computarAcaoMenuMetas(char tecla) {
  switch (tecla) {
    case OPCAO_MENU_MAIN_GOAL:
      telaAtualDisplay = TELA_MENU_META_BASE;
      break;

    case OPCAO_MENU_SUBGOALS:
      indiceMenuSubMetaAtual = 0;
      telaAtualDisplay = TELA_MENU_SUB_METAS;
      break;

    case OPCAO_BACK_TO_MENU:
      telaAtualDisplay = TELA_MENU_PRINCIPAL;
      break;
  }
}

void computarAcaoMenuMetaBase(char tecla) {
  switch (tecla) {
    case OPCAO_CHANGE_MAIN_GOAL:
      telaAtualDisplay = TELA_ALTERAR_META_BASE;
      valorDigitado = metaBase.valor;
      break;

    case OPCAO_BACK_TO_MY_GOALS:
      telaAtualDisplay = TELA_MENU_METAS;
      break;
  }
}

int charToInt(char caracter) {
  switch (caracter) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
  }
}

void computarAcaoMenuSubMetas(char tecla) {
  switch (tecla) {
    case OPCAO_SET_OR_CHANGE_SUBGOAL:
      telaAtualDisplay = TELA_ALTERAR_SUB_META;
      valorDigitado = subMetas[indiceMenuSubMetaAtual].valor;
      break;

    case OPCAO_NEXT_SUBGOAL:
      indiceMenuSubMetaAtual += 1;

      if (indiceMenuSubMetaAtual >= QTDE_MAXIMA_SUB_METAS) {
        indiceMenuSubMetaAtual = 0;
      }
      break;

    case OPCAO_BACK_TO_MY_GOALS:
      telaAtualDisplay = TELA_MENU_METAS;
      break;
  }
}

float adicionarDigitoAoValor(float valor, char tecla) {
  long valorEmCentavos = (long)(valor * 1000) / 10;
  valorEmCentavos = valorEmCentavos * 10 + charToInt(tecla);
  return (float)(valorEmCentavos) / 100;
}

float apagarUltimoDigitoDoValor(float valor) {
  long valorEmCentavos = (long)(valor * 1000) / 10;
  valorEmCentavos = valorEmCentavos / 10;
  return (float)(valorEmCentavos) / 100;
}

void computarMetaBaseDigitada(char tecla) {
  if (ehNumero(tecla)) {
    // Se for um número e o valor não excedeu o valor limite máximo, adiciona no final
    // o valor da tecla digitada digitada
    valorDigitado = adicionarDigitoAoValor(valorDigitado, tecla);

    if (valorDigitado > valorLimiteMaximo) {
      valorDigitado = apagarUltimoDigitoDoValor(valorDigitado);

      telaRetornarDisplay = telaAtualDisplay;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = F("Goals cannot exceed R$999,99");
    }
  }
  else if (ehApagar(tecla)) {
    // Se for a tecla de apagar, remove o último digito da meta digitada
    valorDigitado = apagarUltimoDigitoDoValor(valorDigitado);
  }
  else if (ehConfirmar(tecla)) {
    if (valorDigitado >= valorLimiteMinimo && valorDigitado <= valorLimiteMaximo) {
      telaRetornarDisplay = TELA_MENU_META_BASE;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = F("Main goal changed successfully!");
      metaBase.valor = valorDigitado;

      publicarAtualizacaoMetasMQTT();
    }
    else {
      telaRetornarDisplay = TELA_ALTERAR_META_BASE;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = F("Main goal must be between R$0,01 and R$999,99");
    }
  }
  else if (ehCancelar(tecla)) {
    telaAtualDisplay = TELA_MENU_META_BASE;
  }
}

void computarSubMetaDigitada(char tecla) {
  if (ehNumero(tecla)) {
    // Se for um número e o valor não excedeu valor limite máximo, adiciona no final o
    // valor da tecla digitada digitada
    valorDigitado = adicionarDigitoAoValor(valorDigitado, tecla);

    if (valorDigitado >= metaBase.valor) {
      valorDigitado = apagarUltimoDigitoDoValor(valorDigitado);

      telaRetornarDisplay = telaAtualDisplay;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = "Subgoals cannot exceed " + aplicarMascaraDinheiro(metaBase.valor) + " (main goal)";
    }
  }
  else if (ehApagar(tecla)) {
    // Se for a tecla de apagar, remove o último digito da meta digitada
    valorDigitado = apagarUltimoDigitoDoValor(valorDigitado);
  }
  else if (ehConfirmar(tecla)) {
    if (valorDigitado <= 0 || valorDigitado >= metaBase.valor) {
      telaRetornarDisplay = telaAtualDisplay;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = "Subgoal must be between R$0,01 and " + aplicarMascaraDinheiro(metaBase.valor) + " (main goal)";
    }
    else if (subMetaEhRepetida()) {
      telaRetornarDisplay = telaAtualDisplay;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = "The subgoal " + aplicarMascaraDinheiro(valorDigitado) + " already exists";
    }
    else {
      telaRetornarDisplay = TELA_MENU_SUB_METAS;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = F("Subgoal changed successfully!");
      subMetas[indiceMenuSubMetaAtual].valor = valorDigitado;
      subMetas[indiceMenuSubMetaAtual].foiAtingida = valorDigitado >= valorGuardado;

      publicarAtualizacaoMetasMQTT();
    }
  }
  else if (ehCancelar(tecla)) {
    telaAtualDisplay = TELA_MENU_SUB_METAS;
  }
  else if (ehDeletar(tecla)) {
    telaRetornarDisplay = TELA_MENU_SUB_METAS;
    telaAtualDisplay = TELA_MENSAGEM;
    mensagemDisplay = F("Subgoal deleted successfully!");
    subMetas[indiceMenuSubMetaAtual].valor = 0;
    subMetas[indiceMenuSubMetaAtual].foiAtingida = false;

    publicarAtualizacaoMetasMQTT();
  }
}

void computarValorDepositoDigitado(char tecla) {
  if (ehNumero(tecla)) {
    // Se for um número e o valor não excedeu valor limite máximo, adiciona no final o
    // valor da tecla digitada digitada
    valorDigitado = adicionarDigitoAoValor(valorDigitado, tecla);

    if (valorDigitado + valorGuardado > valorLimiteMaximo) {
      valorDigitado = apagarUltimoDigitoDoValor(valorDigitado);

      telaRetornarDisplay = telaAtualDisplay;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = F("The total amount cannot exceed R$999,99");
    }
  }
  else if (ehApagar(tecla)) {
    // Se for a tecla de apagar, remove o último digito da meta digitada
    valorDigitado = apagarUltimoDigitoDoValor(valorDigitado);
  }
  else if (ehConfirmar(tecla)) {
    if (valorDigitado < valorLimiteMinimo || valorDigitado + valorGuardado > valorLimiteMaximo) {
      telaRetornarDisplay = telaAtualDisplay;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = "Your deposit must be between R$0,01 and " + aplicarMascaraDinheiro(valorLimiteMaximo - valorGuardado);
    }
    else {
      telaRetornarDisplay = TELA_MENU_MEU_DINHEIRO;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = F("Money deposited successfully!");
      valorGuardado += valorDigitado;

      mensagemPublicarMQTT = "You deposited " + aplicarMascaraDinheiro(valorDigitado) + ". Now you have " + aplicarMascaraDinheiro(valorGuardado) + ".";
      
      publicarNoTopicoMQTT(topicoMQTT, mensagemPublicarMQTT);

      valorDigitado = 0;
      atualizarMetasAtingidas();
    }
  }
  else if (ehCancelar(tecla)) {
    telaAtualDisplay = TELA_MENU_MEU_DINHEIRO;
  }
}

void atualizarMetasAtingidas() {
  if (valorGuardado < metaBase.valor) {
    metaBase.foiAtingida = false;
  }

  if (!metaBase.foiAtingida && valorGuardado >= metaBase.valor) {
    metaBase.foiAtingida = true;

    for (byte indice = 0; indice < QTDE_MAXIMA_SUB_METAS; indice++) {
      subMetas[indice].foiAtingida = true;
    }

    telaRetornarDisplay = TELA_MENU_MEU_DINHEIRO;
    telaAtualDisplay = TELA_MENSAGEM;
    mensagemDisplay = "Congratulations: Your main goal (" + aplicarMascaraDinheiro(metaBase.valor) + ") has been reached!";

    publicarNoTopicoMQTT(topicoMQTT, mensagemDisplay);
  }
  else {
    float maiorSubMetaAtingida = 0;

    for (byte indice = 0; indice < QTDE_MAXIMA_SUB_METAS; indice++) {
      if (valorGuardado < subMetas[indice].valor) {
        subMetas[indice].foiAtingida = false;
      }

      if (!subMetas[indice].foiAtingida && valorGuardado >= subMetas[indice].valor) {
        subMetas[indice].foiAtingida = true;

        if (maiorSubMetaAtingida < subMetas[indice].valor) {
          maiorSubMetaAtingida = subMetas[indice].valor;
        }
      }
    }

    if (maiorSubMetaAtingida > 0) {
      telaRetornarDisplay = TELA_MENU_MEU_DINHEIRO;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = "Congratulations: Your " + aplicarMascaraDinheiro(maiorSubMetaAtingida) + " subgoal has been reached!";

      publicarNoTopicoMQTT(topicoMQTT, mensagemDisplay);
    }
  }
}

void computarAcaoConfirmarSaqueDinheiro(char tecla) {
  switch (tecla) {
    case OPCAO_CONFIRM_WITHDRAW:
      telaRetornarDisplay = TELA_MENU_MEU_DINHEIRO;
      telaAtualDisplay = TELA_MENSAGEM;
      mensagemDisplay = aplicarMascaraDinheiro(valorGuardado) + " withdrawn successfully!";

      publicarNoTopicoMQTT(topicoMQTT, mensagemDisplay);

      valorGuardado = 0;
      atualizarMetasAtingidas();
      break;

    case OPCAO_CANCEL_WITHDRAW:
      telaAtualDisplay = TELA_MENU_MEU_DINHEIRO;
      break;
  }
}

bool subMetaEhRepetida() {
  for (byte indice = 0; indice < QTDE_MAXIMA_SUB_METAS; indice++) {
    bool naoEhSubMetaQueEstaEditando = indice != indiceMenuSubMetaAtual;
    bool subMetaPossuiMesmoValor = subMetas[indice].valor == valorDigitado;

    if (naoEhSubMetaQueEstaEditando && subMetaPossuiMesmoValor) {
      return true;
    }
  }

  return false;
}

String aplicarMascaraDinheiro(float valor) {
  String strValor = String(((long)(valor * 1000)) / 10);

  while (strValor.length() < 3) {
    strValor = "0" + strValor;
  }

  String retorno = strValor;

  int8_t indice = strValor.length() - 1;

  byte distanciaFinal;

  while (indice >= 0) {
    distanciaFinal = strValor.length() - 1 - indice;

    if (distanciaFinal == 2) {
      retorno = retorno.substring(0, indice + 1) + F(",") + retorno.substring(indice + 1, retorno.length());
    }
    else if ((int)((distanciaFinal - 2) % 3) == 0) {
      retorno = retorno.substring(0, indice + 1) + F(".") + retorno.substring(indice + 1, retorno.length());
    }

    indice -= 1;
  }

  return "R$" + retorno;
}