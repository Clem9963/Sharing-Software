#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ncurses.h>

#define TAILLE_BUF 1000
#define TAILLE_PSEUDO 16
#define PORT 54888

char *demandeIP();
char *demandePseudo();
int connect_socket(char *adresse, int port);
void read_serveur(int sock, char *buffer, char **conversation, int *ligne, WINDOW *fenHaut, WINDOW *fenBas);
void write_serveur(int sock, char *buffer);
void ecritDansConv(char *buffer, char **conversation, int *ligne, WINDOW *fenHaut, WINDOW *fenBas);
char **initConv();
void concatener(char *buffer, char *pseudo);
void initInterface(WINDOW *fenHaut, WINDOW *fenBas);

int main(int argc, char *argv[]){
	char buffer[TAILLE_BUF];
	int sock, lig = 0;
	int *ligne = &lig;
	char * pseudo = demandePseudo();
	char *ipAdresse = demandeIP();
	fd_set readfds;	
	

	//Initialisation de l'interface graphique
	WINDOW *fenHaut, *fenBas;
	initscr();
	fenHaut = subwin(stdscr, LINES - 3, COLS, 0, 0);
    fenBas = subwin(stdscr, 3, COLS, LINES - 3, 0);
	initInterface(fenHaut, fenBas);

    //creation de la conversation
    char **conversation = initConv();
	
	sock = connect_socket(ipAdresse, PORT);	//connexion au serveur
	write_serveur(sock, pseudo); //envoi le pseudo au serveur
	
	//libere la memoire du pseudo et adresse ip car plus besoin
	free(pseudo);
	free(ipAdresse);

	//boucle du chat
	while(1)
	{
		move(LINES - 2, 17);//se positionn au bon endroit pour ecrire le message
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		FD_SET(sock, &readfds);

		if(select(sock + 1, &readfds, NULL, NULL, NULL) == -1)
		{
			perror("select()");
			exit(-1);
		}

		if(FD_ISSET(sock, &readfds))
		{
			read_serveur(sock, buffer, conversation, ligne, fenHaut, fenBas);
		}
		else if(FD_ISSET(STDIN_FILENO, &readfds))
		{
			getstr(buffer);
			buffer[strlen(buffer)] = '\0';
			
			// ****todo: faire si le message est stop

			write_serveur(sock, buffer); //envoi le message au serveur
			
			//ecrit le message dans la conversation en rajouter "vous" devant
			concatener(buffer, "Vous");
			ecritDansConv(buffer, conversation, ligne, fenHaut, fenBas);
        }
    }

	close(sock);

	return 0;
}

void initInterface(WINDOW *fenHaut, WINDOW *fenBas){
    box(fenHaut, ACS_VLINE, ACS_HLINE);
    box(fenBas, ACS_VLINE, ACS_HLINE);
    mvwprintw(fenHaut,1,1,"Messages : ");
    mvwprintw(fenBas, 1, 1, "Votre message : ");
}

int connect_socket(char *adresse, int port){
	int sock = socket(AF_INET, SOCK_STREAM, 0); //créé un socket TCP

	if( sock == -1)
	{
		perror("socket()");
		exit(-1);
	}

	struct hostent* hostinfo = gethostbyname(adresse); // infos du serveur
	if(hostinfo == NULL)
	{
		perror("gethostbyname()");
		exit(-1);
	}
	
	struct sockaddr_in sin; //structure qui possède toutes les infos pour le socket
	sin.sin_addr = *(struct in_addr*) hostinfo->h_addr; // on spécifie l'addresse
	sin.sin_port = htons(port); //le port 
	sin.sin_family = AF_INET; //et le protocole (AF_INET pour IP)
	
	if(connect(sock, (struct sockaddr*) &sin, sizeof(struct sockaddr)) == -1)
	{
		perror("connect()");
		exit(-1);
	}

	return sock;
}

//initialise la conversation
char **initConv(){
	int i;
	char **conv = malloc(sizeof(char *) * (LINES - 6));

    for (i = 0; i < LINES - 6; i++)
        conv[i] = malloc(sizeof(char)* TAILLE_BUF);

    return conv;
}

void read_serveur(int sock, char *buffer, char **conversation, int *ligne, WINDOW *fenHaut, WINDOW *fenBas){
	int taille_recue;

	taille_recue = recv(sock, buffer, TAILLE_BUF, 0);

	if(taille_recue == -1)
	{
		endwin();
		perror("recv()");
		exit(-1);
	}
	//pour gerer la deconnexion du serveur
	else if(taille_recue == 0)
	{
		endwin();
		printf("\nla connexion au serveur a été interrompue\n\n");
		exit(-1);
	}
	else
	{
		//ecrit le message dans la conversation
		buffer[taille_recue] = '\0';
		ecritDansConv(buffer, conversation, ligne, fenHaut, fenBas);
	}	
}

//envoi message au serveur
void write_serveur(int sock, char *buffer){
	if(send(sock, buffer, strlen(buffer), 0) == -1)
	{
		perror("send()");
		exit(-1);
	}
}

//demande uen adresse ip
char *demandeIP(){
	char *adresseIP = malloc(sizeof(char) * 16);

	printf("\nAdresse IP du serveur : ");
    scanf("%s",adresseIP);
    
    if(strlen(adresseIP) > 15 || strlen(adresseIP) < 7)
    {
        fprintf( stderr, "Mauvaise adresse IP...\n" );
		exit(-1);
    }

    return adresseIP;
}

//demande un pseudo
char *demandePseudo(){
	char *pseudo = malloc(sizeof(char) * 16);

	printf("\nVotre pseudo : ");
    scanf("%s",pseudo);

    while(strlen(pseudo) > 15)
    {
    	printf("\nVotre pseudo est trop long (au max 15 caracteres)");
    	printf("\nVotre pseudo : ");
    	scanf("%s",pseudo);
    }

    return pseudo;
}

//ecrit un message dans la conversation
void ecritDansConv(char *buffer, char **conversation, int *ligne, WINDOW *fenHaut, WINDOW *fenBas){
	int i, j;

	//****todo: faire si message trop long

	//on ecrit le nouveau message dans le chat
    if(*ligne < LINES - 6)
    {
        strcpy(conversation[*ligne], buffer);
    	mvwprintw(fenHaut,*ligne + 2, 1,conversation[*ligne]);
    	(*ligne)++;
    }
    //sinon on scroll de un vers le bas
    else
    {
    	//effacer l'affichage de la conversation
    	for(i = 0; i < LINES - 6; i++)
    		for(j = 0; j < strlen(conversation[i]); j++)
    			mvwprintw(fenHaut, 2+i , 1+j," ");

    	//decalage vers le haut
    	for(i = 0; i < LINES - 7; i++)
    	{
    		strcpy(conversation[i], conversation[i+1]);
    		mvwprintw(fenHaut, 2+i , 1,conversation[i]);
		}

		//ecriture du nouveau message en bas
    	strcpy(conversation[LINES - 7], buffer);
    	mvwprintw(fenHaut, 2 + LINES - 7, 1,conversation[LINES - 7]);
    }

    //supprimer le message "message trop long" si jamais il y etait avant
    box(fenBas, ACS_VLINE, ACS_HLINE);
	//supprimer le message qui a ete mis
    for (i = 0; i < strlen(buffer); i++)
    	buffer[i] = ' ';

	mvwprintw(fenBas, 1, 1, "Votre message : %s", buffer);
	move(LINES - 2, 17);

    wrefresh(fenHaut);
    wrefresh(fenBas);
}

//concatene le pseudo dans le buffer pour faire "pseudo : message" (+ propre)
void concatener(char *buffer, char *pseudo){
	char *temp = malloc(sizeof(char) * (TAILLE_BUF + TAILLE_PSEUDO));
	sprintf(temp, "%s : %s", pseudo, buffer);
	strcpy(buffer, temp);
	free(temp);
}