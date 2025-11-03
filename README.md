# 🔍 Scanner de Portas TCP em C

## 📘 Descrição
Este projeto implementa um **scanner de portas TCP** em linguagem C.  
O programa testa uma faixa de portas (exemplo: de 100 a 200) em um determinado **endereço IP**,  
exibindo na tela quais portas estão abertas e salvando o resultado em um arquivo de texto.

## ⚙️ Funcionalidades
- Testa uma faixa configurável de portas (por exemplo: 100–200);
- Mostra o status das portas no terminal;
- Grava os resultados no arquivo `resultado.txt`;
- Simples de executar e fácil de entender para estudos de redes e sockets em C.

## 🧠 Tecnologias utilizadas
- Linguagem C
- Biblioteca de Sockets POSIX (`sys/socket.h`, `arpa/inet.h`)

## 🖥️ Como compilar
No terminal (Linux/macOS):

```bash
gcc scanner_portas.c -o scanner
