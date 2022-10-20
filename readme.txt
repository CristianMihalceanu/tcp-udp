Mihalceanu Cristian-Ionut
Grupa 321CC
Tema 2 Aplicatie client server TCP si UDP pentru gestionarea mesajelor

structurile definite sunt urmatoarele:
    tcp_message 
        -> contine informatia trimisa de clientul tcp catre server
        care se ocupa cu subscribing-ul
    server_message
        -> contine informatia primita de la clientul udp
        si decodata ulterior de server

structurile de date utilizate:
    unordered_map clients 
        -> cheie: topic
        -> valoare: vector de clienti(id-uri) abonati la topic
    unorderd_map idmap
        ->cheie: id
        ->valoare: file descriptor-ul(socket) asociat id-ului


Server.cc

    Se deschid doua socket-uri: 
        -> udpSocket, pe care se vor primi datagrame de la clientii udp
        -> tcpSocket, pe care se vor primi conexiuniile clientilor tcp
    Inainte de a se realia bind-ul socket-ului tcp se va dezactiva algoritmul
    lui Nagle

    Vom face multiplexarea intre stdin, udpSocket si tcpSocket ulterior
    adaugand in fd_set-ul read_fds socket-urile asociate clientilor tcp
    conectati

    Flow-ul serverului: 
        Clientii tcp se conecteaza cu un id unic, acestia se pot abona si dezabona 
        la diferite topicuri(transmitand structuri de tip tcp_message)
        Clientii udp trimit pachete de forma {topic, type, content} unde 
        serverul va decripta informatia din content pe baza type-ului
        Serverul trimite informatia decriptata clientilor tcp ascoiati topic-ului
        Serverul se poate inchide cu comanda "exit" data de la tastatura

Subscriber.cc

    Se deschide un socket pentru a se realiza conexiunea la server
    iar clientul trimite id-ul dat ca argument de apel al functiei main
    pentru a se autentifica

    Vom face multiplexarea intre stdin si socket-ul deschis pentru comunicarea
    cu serverul
    
    Flow-ul clientului:
        Clientul se poate abona la diferite topicuri introducand la stdin
        comanda "subscribe topic sf"
        Acesta primeste structuri de tip server_message de la server ce 
        contin informatia data de clientii udp care au transmis pachete 
        cu un topic la care este asociat clientul tcp curent.
