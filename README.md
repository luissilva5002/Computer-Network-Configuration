  
  
  
  

Computer Network

2º Projeto Laboratorial

  
  

Redes de Computadores:

Grupo H Turma 5

Francisco Teixeira (up202006111@up.pt)

Guilherme Ferreira (up202207524@up.pt)

Luís Silva(up202304347@up.pt)

  
  

FEUP Porto, dezembro de 2025

Sumário

O objetivo deste projeto na disciplina de Computer Networks foi duplo: desenvolver uma aplicação de download de ficheiros usando o protocolo FTP em C com bibliotecas padrão do Unix, e configurar uma rede de computadores completa, passo a passo, no laboratório da FEUP.

O desenvolvimento da aplicação de download permitiu adquirir experiência prática com o protocolo FTP, incluindo o estabelecimento de ligações de controlo e de dados, gestão da autenticação e transferência de ficheiros. Ao mesmo tempo, a configuração da rede em múltiplos dispositivos proporcionou uma compreensão prática do funcionamento das redes, dos comandos e das configurações, reforçando o conhecimento adquirido nas camadas Medium Access Control, Network e Transport.

  

Introdução

Este relatório apresenta os objetivos, o desenho e os resultados do segundo projeto da disciplina de Computer Networks. O projeto consistiu na criação de uma aplicação de download FTP e na configuração de uma rede segmentada no laboratório, simulando cenários de conectividade realistas.

O relatório está estruturado da seguinte forma:

-   Download Application: Descreve o desenho e a implementação do cliente FTP, incluindo parsing de URLs, estabelecimento de ligações, autenticação, transferência de dados em modo passivo e armazenamento de ficheiros.
    
-   Network Configuration: Detalha a configuração passo a passo da rede do laboratório, incluindo comandos, capturas Wireshark e análise teórica, organizada nas seguintes experiências:
    

1.  Experiment 1 – Configure an IP Network
    
2.  Experiment 2 – Implement two Bridges in a Switch
    
3.  Experiment 3 – Configure a Router in Linux
    
4.  Experiment 4 – Configure a Commercial Router and Implement NAT
    
5.  Experiment 5 – Domain Name System (DNS)
    
6.  Experiment 6 – TCP Connections
    

-   Conclusion: Resume os resultados e reflete sobre os conhecimentos e competências adquiridas durante o projeto.
    

O apêndice inclui o código-fonte da aplicação de download, bem como os comandos e as capturas de rede utilizadas nas diferentes experiências.

3 Download Application

A primeira parte deste projeto consistiu no desenvolvimento de um cliente FTP simples, implementado em linguagem C, capaz de efetuar o download de um único ficheiro a partir do respetivo URL, em conformidade com o standard RFC 1738.

  

3.1 Architecture

A implementação foi realizada num único ficheiro de código-fonte, lab2download.c, que contém diversas funções auxiliares e estruturas essenciais para o correto funcionamento da aplicação. As mais relevantes incluem:

-   FtpUrl – Estrutura que armazena os componentes do URL FTP, nomeadamente o nome de utilizador e a password utilizados na autenticação, o domínio do servidor e o caminho do ficheiro.
    
-   Message – Estrutura responsável por armazenar informação sobre cada linha da resposta do servidor, incluindo o código de resposta e a indicação de linha final.
    
-   parse\_url() – Cria uma instância de FtpUrl a partir do URL fornecido, verificando a sua validade e extraindo todos os componentes.
    
-   get\_socket\_fd\_addr() e get\_socket\_fd\_host() – Estabelecem a ligação do cliente ao servidor usando o endereço IP ou o hostname e a porta fornecida, retornando o descritor da socket.
    
-   read\_message() – Lê uma linha da resposta do servidor e preenche a estrutura Message correspondente.
    
-   read\_end() – Lê todas as linhas da resposta do servidor até à linha final, armazenando a informação num argumento apropriado.
    
-   check\_code() – Verifica se o código de resposta recebido corresponde ao esperado, emitindo uma mensagem de erro em caso de discrepância.
    
-   send\_command() – Envia um comando FTP para a socket de controlo.
    
-   parse\_pasv\_response() – Interpreta a resposta ao comando PASV para extrair o IP e a porta onde ocorrerá a transferência de dados do ficheiro.
    

  

3.2 Application Execution and Flow

Após a compilação da aplicação com o comando make, é possível executá-la da seguinte forma:

./lab2download <URL>

O fluxo de execução da aplicação segue os passos abaixo descritos. Em qualquer etapa em que ocorra um erro, como um URL inválido ou um código de resposta inesperado do servidor, a execução é interrompida e é apresentada uma mensagem de erro apropriada:

1.  Parsing do URL – O URL fornecido é analisado e interpretado, assegurando que os componentes são válidos. Caso o nome de utilizador ou a password não sejam fornecidos, são utilizados os valores por defeito “anonymous”.
    
2.  Estabelecimento da Ligação – É criada uma ligação de socket ao servidor através da porta padrão 21. Esta ligação de controlo permite enviar comandos FTP e receber respostas do servidor.
    
3.  Login e Obtenção de Informação do Ficheiro – As credenciais são enviadas para autenticar a sessão. Após autenticação bem-sucedida, a aplicação define o modo de transferência binário (com o comando TYPE I) e obtém o tamanho do ficheiro (com o comando SIZE), permitindo monitorizar o progresso da transferência.
    
4.  Modo Passivo e Transferência de Dados – A aplicação entra em modo passivo (com o comando PASV), criando uma nova ligação de socket ao IP e porta fornecidos pelo servidor. Esta ligação de dados permite receber os conteúdos do ficheiro e gravá-los localmente com o mesmo nome.
    
5.  Terminação – Concluída a transferência, a ligação de dados é encerrada. Seguidamente, a ligação de controlo é fechada com o comando QUIT, garantindo uma terminação ordenada da sessão.
    

3.3 Download Report

A aplicação foi testada com sucesso com todos os URLs FTP fornecidos. No caso do ficheiro pipe.txt disponível em ftp://ftp.netlab.fe.up.pt/, os respetivos logs do Wireshark podem ser observados na Figura 1, sendo também apresentado o output do terminal neste exemplo.

Figura 1: Logs do Wireshark de um Download Bem-Sucedido

  

### 4 Configuration of Networks

As subseções seguintes descrevem cada uma das experiências realizadas nas aulas práticas, com enfoque na arquitetura da rede, nos comandos de configuração, nos logs relevantes e na explicação dos objetivos. Como cada experiência inclui também algumas questões, serão fornecidas respostas e explicações correspondentes.

Para referência, a imagem abaixo ilustra a configuração final da rede de computadores, replicada ao longo das experiências. Note-se que os testes e capturas de logs foram realizados no Workbench 6; portanto, os logs apresentarão um “6” onde na figura está representado “Y”. Ao longo do relatório, procurou-se manter as explicações de forma geral, incluindo exemplos para maior clareza.

  

Figura 2: Configuração da Rede de Computadores

  

### 4.1 Experiment 1 – Configure an IP Network

O objetivo desta primeira experiência foi configurar a rede entre dois computadores distintos, Tux122 e Tux123, utilizando um switch Mikrotik. Esta configuração permitiu familiarizarmo-nos com o ambiente de trabalho, atribuir endereços IP e analisar as tabelas ARP.

Os comandos de configuração utilizados encontram-se em Experiment 1 Commands.

O primeiro passo consistiu em ligar as portas Ethernet dos dois computadores ao switch. De seguida, utilizando o comando ifconfig, foram atribuídos os endereços IP na sub-rede correspondente à bancada: 172.16.120.254 para Tux122 e 172.16.120.253 para Tux123. Cada dispositivo possui também um endereço MAC único, consultável com ifconfig.

Com a configuração concluída, a conectividade foi testada através do comando ping. Utilizando o Wireshark, observou-se que os pacotes ICMP Echo Request/Reply contêm os endereços IP de origem e destino, bem como os endereços MAC do próximo salto (next-hop).

No início, como os computadores ainda não conheciam os MACs uns dos outros, a máquina de origem envia um pedido ARP em broadcast, indicando o IP de origem e destino. A máquina com o IP de destino responde com o seu MAC, que é guardado na tabela ARP, evitando pedidos futuros desnecessários.

Os pedidos ARP são identificáveis pelo EtherType: 0x0806 para ARP e 0x0800 para IP. Quando se trata de pacotes IP, o campo “Protocol” indica 1 para ICMP. O tamanho de um frame recebido é determinado pelo payload no cabeçalho Ethernet.

A tabela ARP também contém um valor especial para a interface loopback, uma interface de software que permite à máquina comunicar consigo própria, essencial para testes internos. Estes conceitos podem ser confirmados nos logs de Experiment 1 Logs.

  

Experiment 1 Logs:

-   tux4:
    

-   tux3:
    

-   tux2:
    

  

### 4.2 Experiment 2 – Implement two Bridges in a Switch

O objetivo desta experiência foi implementar duas bridges no switch Mikrotik: bridge120, que conecta Tux122 e Tux123, e bridge121, que conecta Tux124. Este exercício permitiu aprender a criar bridges e verificar que dispositivos em sub-redes diferentes não se comunicam diretamente.

Os comandos utilizados encontram-se em Experiment 2 Commands.

Primeiro, configurou-se a ligação em Tux124 com ifconfig. De seguida, na consola do switch Mikrotik, criaram-se as bridges bridge120 e bridge121, atribuindo-se as portas Ethernet correspondentes a cada dispositivo.

Para verificar as ligações, capturaram-se logs em todos os computadores e executaram-se broadcasts de ping em Tux122 e depois em Tux124. Como esperado, os dispositivos na mesma bridge comunicam (Tux122 liga a Tux123), enquanto dispositivos em bridges diferentes não (Tux122 não liga a Tux124 e vice-versa). Conclui-se assim que existem dois domínios de broadcast distintos: bridge120 (Tux122 e Tux123) e bridge121 (Tux124).

Os logs deste experimento encontram-se em Experiment 2 Logs.

  

Experiment 2 Logs:

-   tux4:
    

-   tux3:
    

-   tux2:
    

  

### 4.3 Experiment 3 – Configure a Router in Linux

O objetivo desta experiência foi estender a configuração anterior, usando Tux123 como router entre as duas bridges. Esta configuração permite compreender o funcionamento do routing, conectar sub-redes diferentes e analisar IPs, MACs e tabelas ARP e de encaminhamento.

Os comandos encontram-se em Experiment 3 Commands.

Primeiro, configurou-se uma nova porta Ethernet em Tux123 e adicionou-se à bridge121, que liga a Tux124, especificando a porta correta no switch. Para permitir que Tux123 funcione como router entre as sub-redes, ativou-se o IP forwarding e desativou-se a opção ICMP echo-ignore-broadcast.

Foram adicionadas rotas estáticas:

-   Em Tux122 para a sub-rede de Tux124: 172.16.121.0/24, gateway 172.16.120.254
    
-   Em Tux124 para a sub-rede de Tux122: 172.16.120.0/24, gateway 172.16.121.253
    

Assim, quando Tux122 tenta ligar a Tux124, o pacote passa por Tux123 (gateway) até chegar ao destino, e vice-versa.

Após a configuração, o ping entre Tux122 e Tux124 funciona corretamente. Limpar as tabelas ARP gera pedidos broadcast iniciais para resolução de MAC, conforme observado nos logs.

As tabelas de encaminhamento de todos os Tux indicam o endereço IP de destino, o next-hop e a interface utilizada. No caso de Tux123, é possível verificar como os pacotes são encaminhados entre as duas sub-redes.

Os logs deste experimento encontram-se em Experiment 3 Logs.

  

Experiment 3 Logs:

-   tux3:
    

-   tux2:
    

-   tux1:
    

  
  

4.4 Experiment 4 – Configure a Commercial Router and Implement NAT

O objetivo desta quarta experiência foi dar continuidade à configuração das duas bridges da experiência anterior, adicionando agora um router Mikrotik ligado à bridge121 do switch Mikrotik (que contém Tux124 e Tux123). Este router Mikrotik permitirá também a ligação da rede à Internet através de NAT, sendo necessário atualizar as rotas em todos os Tuxes. Esta experiência permite aprender a configurar o router Mikrotik, ligar a rede à Internet, implementar NAT e aprofundar conhecimentos sobre routing.

Os comandos utilizados encontram-se em Experiment 4 Commands.

O primeiro passo consistiu em ligar o router à rede do laboratório, com NAT ativado por omissão, e depois ligá-lo ao switch Mikrotik. De seguida, através da consola do switch, ligou-se o router à bridge121, de forma semelhante ao que foi feito nas experiências anteriores.

Posteriormente, configuraram-se e verificaram-se todas as rotas na rede, conforme indicado em Experiment 4 Commands. Seguindo a configuração da rede do lado esquerdo para o direito:

-   Tux122: ligação às sub-redes 172.16.121.0 (bridge121) e 172.16.1.0 (externa) através do gateway 172.16.120.254 (Tux123)
    
-   Tux123: ligação à sub-rede 172.16.1.0 (externa) através do gateway 172.16.121.254 (Router)
    
-   Tux124: ligação à sub-rede 172.16.1.0 (externa) via gateway 172.16.121.254 (Router) e à sub-rede 172.16.120.0 (bridge120) através do gateway 172.16.121.253 (Tux123)
    
-   Router: ligação à sub-rede 172.16.120.0 (bridge120) através do gateway 172.16.121.253 (Tux123)
    

Com esta configuração, é possível fazer ping entre qualquer máquina da rede.

O primeiro teste consistiu em alterar a configuração de Tux124, desativando inicialmente os ICMP redirects e alterando a rota para a sub-rede 172.16.120.0 (bridge120) do gateway 172.16.121.253 (Tux123) para o gateway 172.16.121.254 (Router). Neste cenário, ao executar ping de Tux124 para Tux122, os pacotes seguem a rota: Tux124 → Router → Tux123 → Tux122, demonstrando que, apesar de existir uma rota mais curta, os pacotes percorrem o caminho mais longo devido à desativação dos redirects. Quando os redirects são reativados, os pacotes passam a seguir diretamente Tux124 → Tux123 → Tux122, o que pode ser verificado com traceroute.

Outro teste consistiu em executar ping para o servidor FTP a partir de Tux122, com NAT ativado e desativado. O NAT permite que um conjunto de IPs privados seja traduzido para um único IP público, acessível, por exemplo, a partir da Internet, evitando colisões. Como esperado, com NAT ativo, o ping funciona corretamente; sem NAT, os pacotes chegam ao servidor, mas a resposta não consegue chegar a Tux122, pois não é possível rotear o IP privado.

Os logs desta experiência encontram-se em Experiment 4 Logs.

  

4.5 Experiment 5 – Domain Name System (DNS)

O objetivo desta quinta experiência foi configurar o Domain Name System (DNS) em todas as máquinas da rede. Isto permitiu compreender o funcionamento do DNS e a resolução de nomes em endereços IP, utilizando o serviço DNS do Netlab.

Os comandos utilizados encontram-se em Experiment 5 Commands.

O procedimento consistiu em adicionar services.netlab.fe.up.pt 10.227.20.3 ao ficheiro /etc/resolv.conf de todos os Tuxes. Com esta configuração, todos os dispositivos podem traduzir nomes legíveis por humanos (como google.com ou archlinux.org) nos respetivos endereços IP públicos utilizados na rede.

Quando um endereço é acedido, é enviado um DNS Query aos nameservers definidos em /etc/resolv.conf, que devolvem a resposta em DNS Query Response. Ambos os pacotes seguem a mesma estrutura de header (Transaction ID, Flags, Questions, Answers, Authority, Additional), seguidos de Queries (Name, Type, Class). A resposta contém ainda a secção Answers, com Time To Live, Data Length e Data (resultado da consulta).

Pelos testes realizados com ping, concluímos que google.com possui o IP 142.250.200.142 e archlinux.com o IP 95.217.163.246. Os logs completos podem ser analisados em Experiment 5 Logs.

4.6 Experiment 6 – TCP Connections

O objetivo desta sexta experiência foi testar a aplicação de download desenvolvida, transferindo um ficheiro a partir do servidor FTP para Tux122. Esta experiência permitiu rever os conceitos implementados na aplicação FTP e verificar a correta configuração da rede, bem como observar a troca de pacotes TCP.

Os comandos utilizados encontram-se em Experiment 6 Commands.

Primeiro, compilou-se a aplicação em Tux122 e realizou-se a transferência de um ficheiro do servidor FTP, verificando a sua integridade. Inicialmente, é estabelecida uma ligação TCP de controlo. A aplicação troca comandos com o servidor (login, obtenção do tamanho do ficheiro, etc.) e depois estabelece uma segunda ligação TCP para transferência dos dados.

Nos logs, é possível observar as três fases do TCP: estabelecimento da ligação, transferência de dados e terminação da ligação. Também se verifica o mecanismo ARQ (Automatic Repeat Request), que garante a transmissão fiável de pacotes, e o controlo de congestionamento, com aumento gradual da janela de transferência (additive increase) e redução drástica em caso de erro (multiplicative decrease). O gráfico de throughput evidencia este padrão de serra.

Num teste adicional, verificou-se o impacto no throughput quando dois Tuxes fazem downloads simultâneos. Ao iniciar um download em Tux122 e outro em Tux124 a meio da transferência, observou-se uma ligeira redução do throughput do servidor, conforme indicado no gráfico de throughput.

5 Conclusion

Em conclusão, este projeto permitiu aprofundar conhecimentos fundamentais em redes de computadores, incluindo endereços MAC e IP, bridges, switches, routers, tabelas ARP e de encaminhamento, Network Address Translation, Domain Name System, protocolo TCP e configuração de redes, com foco na camada de rede.

O desenvolvimento da aplicação FTP permitiu experiência prática com sockets, bibliotecas C e comunicação com servidores FTP.

A configuração laboratorial da rede com comandos apropriados e a captura de logs com Wireshark revelaram-se essenciais para consolidar competências práticas e compreender o comportamento dos protocolos.

Globalmente, os objetivos do projeto foram atingidos, proporcionando perceção prática dos protocolos de rede e da configuração de redes de computadores.

A Appendix

A.1 Download Application

A.1.1 Source Code

Código completo da aplicação FTP download:

  
  
  

  
  

  
  
  

  
  

  
  

  

A.1.2 Download Example

A.2 Experiments commands to configure Tux’s

A.2.1 Tux122 Commands

A.2.2 Tux123 Commands

  

A.2.3 Tux124 Commands

A.2.4 Switch and Router Configuration Commands