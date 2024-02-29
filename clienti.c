#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#define Length_Message 1100
#define Length_Username 40
#define Length_Prieten 40
#define Size_Message 1000
#define Size_Password 100
#define nthreads 100
#define Key_Length 32 // 256 biti

const char *key="iustin112iustin112iustin112iustin1";  //32 caractere

/* codul de eroare returnat de anumite apeluri */
extern int errno;

int PORT;
int loop = 0;
int sd = 0;
int Grup=0;
int Privat=0;
char Username[Length_Username];
char Password[Size_Password];

int check(int result, const char *error_message) {
  if (result < 0) {
    perror(error_message);
    return (errno);
  }

  return 0;
}

char* to_lower(char* a)
{
  for(int i=0; i<strlen(a); i++)
  {
    if(a[i]>='A' && a[i]<='Z')
    {
    a[i]=tolower(a[i]);
    }
  }
  return a;
}

void Delet_endl(char *text, int length_text) {
  for (int i = 0; i < length_text; i++)
    if (text[i] == '\n') {
      text[i] = '\0';
      break;
    }
}

char *Encrypt_Password(char *password)
{

  char *encrypt_password =(char *)malloc((Key_Length + 1) * sizeof(char));

  for (int i = 0; i < Key_Length; i++) {
    encrypt_password[i] = password[i] ^ key[i];
  }
  encrypt_password[strlen(password)] = '\0';

  return encrypt_password;
}

void Account_History(char *username)
{

  char filename[45];
  sprintf(filename, "%s_Messages.txt", username);

  FILE* file=fopen(filename, "r");
  if(file==NULL)
    perror("[Server]Eroare la deschiderea fisierului unui user");

  char line[4000];
  while(fgets(line, sizeof(line), file)!=NULL)
  {
    printf("%s", line);
  }

  fclose(file);

}

void Send_Message_Thread() {
  char Message[Length_Message];
  char Text[Length_Message];
  bzero(Message, Length_Message);
  bzero(Text, Length_Message);

  while (1)
  {
      fgets(Message, Length_Message, stdin);
      Delet_endl(Message, Length_Message);

    if (strcasecmp(Message, "exit") ==0 || strcasecmp(Message, "deconectare") ==0) {
      send(sd, Message, strlen(Message), 0);
      loop = 1;
      break;
    }
    else
    if (Grup==1)
    {
      snprintf(Text, sizeof(Text), " %s: %.998s \n", Username, Message);
      send(sd, Text, strlen(Text), 0);
    }
    else
    if (Privat==1)
    {
      snprintf(Text, sizeof(Text), " %s: %.998s \n", Username, Message);
      send(sd, Text, strlen(Text), 0);
    }

     bzero(Message, Length_Message);
    bzero(Text, Length_Message);
  }
}

void Receive_Message_Thread() {
  char Message[Length_Message];
  bzero(&Message, sizeof(Message));

  while (1) {
    int Data = recv(sd, Message, Length_Message, 0);
    if (Data == 0)
      break;
    else {
      Delet_endl(Message, strlen(Message));
      printf(" %s \n", Message);
    }
    bzero(&Message, sizeof(Message));
  }
}

char *Decryp_Password(char *password) {
  char *decryp_password = (char *)malloc((Key_Length + 1) * sizeof(char));
  
  //aplicam principiul XOR la codificare
  for (int i = 0; i < Key_Length; i++) {
    decryp_password[i] = password[i] ^ key[i];
  }
  decryp_password[strlen(password)] = '\0';

  return decryp_password;
}


/* functie de convertire a adresei IP a clientului in sir de caractere */
char *Conv_addr(struct sockaddr_in address) {
  static char str[25];
  char PORT[7];

  /* adresa IP a clientului */
  strcpy(str, inet_ntoa(address.sin_addr));
  /* portul utilizat de client */
  bzero(PORT, 7);
  sprintf(PORT, ":%d", ntohs(address.sin_port));
  strcat(str, PORT);
  return (str);
}

int Check_User_Passward(char *username, char *password) 
{
  FILE *file = fopen("users.txt", "r+w");
  if (file == NULL) {
    perror("[server]Eroare la deschiderea fisierului users.txt. \n");
    return 0;
  }

  char line[nthreads];
  int ok = 0;

  while (fgets(line, sizeof(line), file)) {
    char candidate_password[Size_Password];
    char candidate_username[Length_Username];
    int status_account;
    sscanf(line, "%s %s %d", candidate_username, candidate_password,
           &status_account);

    if (strcmp(candidate_username, username) == 0 &&
        strcmp(candidate_password, password) == 0) {
      ok = 1;
      break;
    }
  }

  fclose(file);
  return ok;
}

void Add_Username_Password(char *username, char *password) {
  FILE *fisier = fopen("users.txt", "a");

  if (fisier == NULL)
    perror("[server]Eroare la deschiderea fisierului users.txt. \n");

  fprintf(fisier, "%s %s %d\n", username, password, 0);
  fclose(fisier);
}



int main(int argc, char **argv) {
  pthread_t send_msg_thread, recv_msg_thread;
  struct sockaddr_in server_addr;
  char Command[Size_Message];
  char Username_new[Length_Username];
  char Password_new[Size_Password];
  char Grupa_Or_Privet[Size_Message];
  char Prieten[Length_Prieten];

  if (argc != 3) {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return errno;
  }

  PORT = atoi(argv[2]);

  printf("<<<<<< Autentificare or Inregistrare.Choose: >>>>>> \n");
  fgets(Command, Size_Message, stdin);
  Delet_endl(Command, strlen(Command));
  to_lower(Command);

  if (strstr(Command, "inregistrare") !=NULL) {
    printf("~~~~~~~~~~ Inregistrare ~~~~~~~~~\n");
    printf("Introduceti Username-ul: ");
    fgets(Username_new, Length_Username, stdin);
    Delet_endl(Username_new, strlen(Username_new));

    if (strlen(Username_new) == 0) {
      printf("Nu ati introdus Username-ul. :( \n");
      return errno;
    }

    printf("Introduceti parola: ");
    fgets(Password_new, Size_Password, stdin);
    Delet_endl(Password_new, strlen(Password_new));
    strcpy(Password_new,Encrypt_Password(Password_new));

    if (strlen(Password_new) == 0) {
      printf("Nu ati introdus Parola. :( \n");
      return errno;
    }
    if (Check_User_Passward(Username_new, Password_new) == 1)
      printf("  User si parola deja folosita. \n");
    else {
      Add_Username_Password(Username_new, Password_new);
      printf("~~~~~~~~~~ Cont creat cu succes. ~~~~~~~~~~\n");
    }
    printf("~~~~~~~~~~ Autentificare ~~~~~~~~~\n");

    printf("Introduceti Username-ul: ");
    fgets(Username, Length_Username, stdin);
    Delet_endl(Username, strlen(Username));

    if (strlen(Username) == 0) {
      printf("Nu ati introdus Username-ul. :( \n");
      return errno;
    }

    printf("Introduceti parola: ");
    fgets(Password, Size_Password, stdin);
    Delet_endl(Password, strlen(Password));

    if (strlen(Password) == 0) {
      printf("Nu ati introdus Parola. :( \n");
      return errno;
    }

    printf("~~~~~~ Doriti Grup sau Privat Messenger? ~~~~~~ \n");
    fgets(Grupa_Or_Privet, Size_Password, stdin);
    Delet_endl(Grupa_Or_Privet, strlen(Grupa_Or_Privet));

    printf("<------------> WELCOME TO LINUX MESSENGER <------------> \n\n");

    /* cream socketul */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr((argv[1]));
    server_addr.sin_port = htons(PORT);

    /* ne conectam la server */
    check(connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)),
          "[client]Eroare la connect(). \n");

    // Send to server Username
    send(sd, Username, Length_Username, 0);
     //Criptare parola
    strcpy(Password,Encrypt_Password(Password));
    send(sd, Password, Size_Password, 0);
    send(sd, Grupa_Or_Privet, Size_Password, 0);


    if(strcasecmp(Grupa_Or_Privet, "Grup")==0)
    {
      Grup=1;
      printf( " -> Grup:  \n");
    }
    else
    if(strcasecmp(Grupa_Or_Privet, "Privat")==0)
    {
      Privat=1;
      printf( " -> Privat:  \n");
      printf(" Contact: ");
      fgets(Prieten, Length_Prieten, stdin);
      send(sd, Prieten, Length_Prieten, 0);
    }
    else
    {
      printf("Nu ati ales nici-o optiune dintre Grup sau Privat. \n");
      return 0;
    }

     check(pthread_create(&send_msg_thread, NULL, (void *)Send_Message_Thread,
                         NULL),
          "[client]Eroare la trimitere mesajului catre thread. \n");
    check(pthread_create(&recv_msg_thread, NULL, (void *)Receive_Message_Thread,
                         NULL),
          "[client]Eroare la primirea mesajului de la thread. \n");

    while (1) {
      if (loop) {
        printf("M-am deconectat (%s). :( \n", Username);

        char choice[25];
        printf("Iti doresti istoricul(istoric) %s \n", Username);
        fgets(choice, sizeof(choice), stdin);
        Delet_endl(choice, sizeof(choice));

         if(strstr(choice, "istoric")!=NULL)
         {
          printf("Istoricul mesajelor lui %s este: \n", Username);
          Account_History(Username);
         }
        break;
      }
    }

    close(sd);

    return 0;
  }
  else
  if (strstr(Command, "autentificare") !=NULL)
  {
    printf("~~~~~~~~~~ Autentificare ~~~~~~~~~\n");

    printf("Introduceti Username-ul: ");
    fgets(Username, Length_Username, stdin);
    Delet_endl(Username, strlen(Username));

    if (strlen(Username) == 0) {
      printf("Nu ati introdus Username-ul. :( \n");
      return errno;
    }

    printf("Introduceti parola: ");
    fgets(Password, Size_Password, stdin);
    Delet_endl(Password, strlen(Password));

    if (strlen(Password) == 0) {
      printf("Nu ati introdus Parola. :( \n");
      return errno;
    }

    printf("~~~~~~ Doriti Grup sau Privat Messenger? ~~~~~~ \n");
    fgets(Grupa_Or_Privet, Size_Password, stdin);
    Delet_endl(Grupa_Or_Privet, strlen(Grupa_Or_Privet));

    printf("<------------> WELCOME TO LINUX MESSENGER <------------> \n\n");

    /* cream socketul */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr((argv[1]));
    server_addr.sin_port = htons(PORT);

    /* ne conectam la server */
    check(connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)),
          "[client]Eroare la connect(). \n");

    // Send to server Username
    send(sd, Username, Length_Username, 0);
     //Criptare parola
    strcpy(Password,Encrypt_Password(Password));
    send(sd, Password, Size_Password, 0);
    send(sd, Grupa_Or_Privet, Size_Password, 0);


    if(strcasecmp(Grupa_Or_Privet, "Grup")==0)
    {
      Grup=1;
      printf( " -> Grup:  \n");
    }
    else
    if(strcasecmp(Grupa_Or_Privet, "Privat")==0)
    {
      Privat=1;
      printf( " -> Privat:  \n");
      printf(" Contact: ");
      fgets(Prieten, Length_Prieten, stdin);
      send(sd, Prieten, Length_Prieten, 0);
    }
    else
    {
      printf("Nu ati ales nici-o optiune dintre Grup sau Privat. \n");
      return 0;
    }

     check(pthread_create(&send_msg_thread, NULL, (void *)Send_Message_Thread,
                         NULL),
          "[client]Eroare la trimitere mesajului catre thread. \n");
    check(pthread_create(&recv_msg_thread, NULL, (void *)Receive_Message_Thread,
                         NULL),
          "[client]Eroare la primirea mesajului de la thread. \n");

    while (1) {
      if (loop) {
        printf("M-am deconectat (%s). :( \n", Username);

        char choice[25];
        printf("Iti doresti istoricul(istoric) %s \n", Username);
        fgets(choice, sizeof(choice), stdin);
        Delet_endl(choice, sizeof(choice));

         if(strstr(choice, "istoric")!=NULL)
         {
          printf("Istoricul mesajelor lui %s este: \n", Username);
          Account_History(Username);
         }
        break;
      }
    }

    close(sd);

    return 0;
  }
  else
  {
    printf(" Ati introdus o comanda invalida. :");
  }
}
