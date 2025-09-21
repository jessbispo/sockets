# 🖥️ Calculadora Distribuída com Sockets (TCP)

Este projeto implementa uma calculadora distribuída cliente-servidor em C, utilizando sockets TCP. O sistema permite que múltiplos clientes se conectem simultaneamente a um servidor para realizar operações matemáticas de forma distribuída. Esse mini-projeto foi desenvolvido para a minha aula de sistemas distribuidos.

## 🏗️ Arquitetura

O sistema é composto por dois componentes principais:

- **Servidor** → Recebe conexões de clientes, interpreta expressões matemáticas, executa os cálculos e retorna os resultados
- **Cliente** → Lê a entrada do usuário, envia a expressão para o servidor e exibe o resultado

## ⚙️ Funcionalidades

### Conectividade
- Conexão TCP entre cliente e servidor
- Suporte a múltiplos clientes simultâneos usando fork-per-connection
- Encerramento gracioso da conexão com comando `QUIT`

### Formatos de Expressão Suportados
- **Prefixo** → `ADD 10 2`, `MUL 4 3`
- **Infixo** → `10 + 2`, `4 * 3`

### Operações Matemáticas
| Operação | Prefixo | Infixo | Descrição |
|----------|---------|--------|-----------|
| Adição | `ADD` | `+` | Soma dois números |
| Subtração | `SUB` | `-` | Subtrai o segundo do primeiro |
| Multiplicação | `MUL` | `*` | Multiplica dois números |
| Divisão | `DIV` | `/` | Divide o primeiro pelo segundo |

### Tratamento de Erros
- **Entrada inválida** → `ERR invalid expression`
- **Divisão por zero** → `ERR division by zero`
- **Formato incorreto** → `ERR invalid expression`

## 🔄 Fluxo de Comunicação

```
+-------------------+                           +-------------------+
|      Cliente      |                           |      Servidor     |
+-------------------+                           +-------------------+
        |                                               |
        | socket()                                      |
        |---------------------------------------------->|
        |                                               |
        |                      socket()                 |
        |                      bind()                   |
        |                      listen()                 |
        |                                               |
        | connect()                                     |
        |---------------------------------------------->|
        |                                               |
        |                      accept()                 |
        |<----------------------------------------------|
        |                                               |
   ===== Conexão estabelecida via TCP =====
        |                                               |
        | fgets() <- lê expressão do usuário            |
        |                                               |
        | send("ADD 10 2\n")                            |
        |---------------------------------------------->|
        |                                               |
        |                      recv()                   |
        |                      parser("ADD 10 2")       |
        |                      executa: 10 + 2 = 12     |
        |                      send("OK 12\n")          |
        |<----------------------------------------------|
        |                                               |
        | recv("OK 12\n")                               |
        | printf("Servidor: OK 12")                     |
        |                                               |
   ===== Loop até usuário digitar QUIT =====
        |                                               |
        | fgets("QUIT")                                 |
        | send("QUIT")                                  |
        |---------------------------------------------->|
        |                                               |
        |                      recv("QUIT")             |
        |                      fecha conexão            |
        |<----------------------------------------------|
        |                                               |
        | close()                                       |
        |---------------------------------------------->|
        |                                               |
   ===== Conexão encerrada =====
```

## 🚀 Como executar

### 1. Compilar o projeto

Na raiz do repositório, execute:

```bash
make
```

Isso irá gerar dois executáveis:
- `bin/server`
- `bin/client`

### 2. Executar o servidor

Inicie o servidor informando a porta desejada:

```bash
./bin/server <porta>
```

**Exemplo:**
```bash
./server 5050
```

### 3. Executar o cliente

No cliente, informe o host e a porta:

```bash
./client <host> <porta>
```

**Exemplo:**
```bash
./client 127.0.0.1 5050
```

## 📖 Exemplo de uso

```
Cliente> ADD 10 2
Servidor> OK 12.000000

Cliente> 8 * 5
Servidor> OK 40.000000

Cliente> 10 / 0
Servidor> ERR division by zero

Cliente> TESTE
Servidor> ERR invalid expression

Cliente> QUIT
Conexão encerrada.
```

## 📂 Estrutura do Projeto

```
sockets/
├── src/          # Código-fonte (client.c, server.c, etc.)
├── include/      # Arquivos de cabeçalho (proto.h, etc.)
├── Makefile      # Script de compilação
└── README.md     # Documentação do projeto
```

## 🛠️ Tecnologias Utilizadas

- **Linguagem C** - Linguagem principal do projeto
- **Sockets TCP/IP** - Comunicação em rede
- **Funções POSIX** - `read`, `write`, `close`, `fork`, `waitpid`
- **Makefile** - Automatização da compilação
- **Linux** - Desenvolvido e testado em ambiente compatível

## 📋 Requisitos do Sistema

- Compilador GCC
- Sistema operacional Linux/Unix
- Make utility
- Biblioteca POSIX padrão

## 📌 Observações Importantes

- O servidor suporta múltiplos clientes simultaneamente usando o modelo fork-per-connection
- O locale numérico é forçado para C, garantindo que os resultados sempre usem ponto (.) como separador decimal
- O cliente e servidor devem ser executados na mesma rede ou na mesma máquina (localhost)
- O servidor permanece ativo até ser encerrado manualmente (Ctrl+C)
- Cada conexão de cliente é tratada em um processo filho separado

## 🐛 Solução de Problemas

### Erro "Address already in use"
Se o servidor não conseguir iniciar devido ao erro "Address already in use":
```bash
#  use uma porta diferente
./server 8081
```

### Conexão recusada
Certifique-se de que:
- O servidor está rodando antes de tentar conectar o cliente
- A porta especificada está correta
- O firewall não está bloqueando a conexão

## 📄 Licença

Este projeto foi desenvolvido para fins educacionais e está disponível sob a licença MIT.

---

