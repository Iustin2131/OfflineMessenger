/* servTCPConcTh2.c - Exemplu de server TCP concurent care deserveste clientii
   prin crearea unui thread pentru fiecare client.
   Asteapta un numar de la clienti si intoarce clientilor numarul incrementat.
        Intoarce corect identificatorul din program al thread-ului.


   Autor: Lenuta Alboaie  <adria@info.uaic.ro> (c)
*/

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
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#define nthreads 100 // Numarul maxim de clineti pe care il poate suposta  //numarul de threaduri
#define Size_Message 1000
#define Length_Username 40
#define Size_Password 100
#define Length_Prieten 40
#define Key_Length 32 // 256 biti

const char *key="iustin112iustin112iustin112iustin1";  //32 caractere

extern int errno; /* eroarea returnata de unele apeluri */

int Count_Clients = 0;
int thCount = 0;
int Grup=0;
int Privat=0;
int ok=0;

/* portul folosit */
int PORT;

/* Client structure */
typedef struct {
  int sd;
  int thCount;
  char Username[Length_Username];
  char Password[Size_Password];
  char Prieten[Length_Prieten];
  int thCount_Prieten;
  int Status_Privat_Or_Public;
} thData;

thData *threadsPool[nthreads]; // un array de structuri Thread

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER; // variabila mutex ce va fi partajata de threaduri

void Delet_endl(char *text, int length_text) {
  for (int i = 0; i < length_text; i++)
    if (text[i] == '\n') {
      text[i] = '\0';
      break;
    }
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

/* Adaugam un nou client */
void Add_Clinet(thData *New_Client) {
  pthread_mutex_lock(&mlock);

  for (int i = 1; i < nthreads; ++i)
    if (threadsPool[i] == NULL)
    {
      threadsPool[i] = New_Client;
      break;
    }

  pthread_mutex_unlock(&mlock);
}

/* Remove clients to queue */
void Remove_Client(int thCount) {
  pthread_mutex_lock(&mlock);

  for (int i = 1; i < nthreads; ++i)
    if (threadsPool[i] != NULL && threadsPool[i]->thCount == thCount) {
      threadsPool[i] = NULL;
      free(threadsPool[i]);
      break;
    }

  pthread_mutex_unlock(&mlock);
}

void Set_All_Users_Offline()
{
  FILE *file = fopen("users.txt", "r+w");
  if (file == NULL)
    perror("[server]Eroare la deschiderea fisierului users.txt. \n");

  char line[nthreads];
  int current_position;

   while (fgets(line, sizeof(line), file)) {
    char candidate_password[Size_Password];
    char candidate_username[Length_Username];
    int status_account;

    sscanf(line, "%s %s %d", candidate_username, candidate_password,
           &status_account);
      if (status_account == 1) {

      current_position=ftell(file);

        fseek(file, -3, SEEK_CUR); // ne mutam cu o pozitie inapoi pe 0 ca sa l

        fputc('0', file); // suprascrierea
        fflush(file);     // salvarea modificarilor
        fseek(file, current_position,SEEK_SET);

      }
  }

  fclose(file);
}

char *Encrypt_Password(char *password)
{

  char *encrypt_password =(char *)malloc((Key_Length + 1) * sizeof(char));

  for (int i = 0; i < Key_Length; i++) {
    encrypt_password[i] = password[i%sizeof(password)] ^ key[i];
  }
  encrypt_password[strlen(password)] = '\0';

  return encrypt_password;
}


char *Decryp_Password(char *password) {
  char *decryp_password = (char *)malloc((Key_Length + 1) * sizeof(char));

  for (int i = 0; i < Key_Length; i++) {
    decryp_password[i] = password[i%sizeof(password)] ^ key[i];
  }
  decryp_password[strlen(password)] = '\0';

  return decryp_password;
}

void Add_History_Message(char* username, char* message, int situation)
{
  pthread_mutex_lock(&mlock);

  char filename[45];
  sprintf(filename, "%s_Messages.txt", username);

  FILE* file=fopen(filename, "a+");
  if(file==NULL)
    perror("[Server]Eroare la deschiderea fisierului unui user");

  if(situation==1)
  fprintf(file, "-> %s\n", message);
  else
  if(situation==0)
  {
    time_t Current_Time;
    struct tm* Info_Time;
    /*Obtinerea timpului actual*/
    time(&Current_Time);
    //COnvertim intr-o structura tm pentru a lua ce vrem
    Info_Time=localtime(&Current_Time);
    fprintf(file, "[%d:%d:%d] [%d.%d.%d]", Info_Time->tm_hour, Info_Time->tm_min, Info_Time->tm_sec, Info_Time->tm_year+1900, Info_Time->tm_mon+1, Info_Time->tm_mday);
    fprintf(file, " %s \n", message);
  }
  else
  if(situation==-1)
  {

  }
   fclose(file);

  pthread_mutex_unlock(&mlock);
}

int Online_Offline_User(char* username)
{
  int ok=0;
  pthread_mutex_lock(&mlock);

  for(int i=1; i<nthreads; i++)
  {
  if(threadsPool[i]!=NULL && strcmp(threadsPool[i]->Username, username)==0)
    {
      ok=1;
      break;
    }
  }

  pthread_mutex_unlock(&mlock);
  return ok;
}


void Save_Offline_message(char* sender,char* recipient, char* text)
{
  FILE* file=fopen("Offline_Messages.txt","a");
   if(file==NULL)
    perror("[Server]Eroare la deschiderea fisierului Offline_Messages.");

   if(fprintf(file,"%s -> %s\n", recipient, text)<0)
   {
    printf("[Server]Eroare la scrierea in Offline_Messages. \n ");
   }
   if(fclose(file)!=0)
   {
     printf("[Server]Eroare la inchiderea in Offline_Messages. \n ");
   }
}

void Transfer_Date_Fisiere(char* file_1, char* file_2)
{
  FILE* file1=fopen(file_1, "r");
  if(file1==NULL)
    perror("[Server]Eroare la deschiderea fisierului.");

  FILE* file2=fopen(file_2, "w");
  if(file1==NULL)
    perror("[Server]Eroare la deschiderea fisierului.");


  char line[4000];
  size_t Bytes_Read;

  while((Bytes_Read=fread(line,1, sizeof(line),file1))>0)
   {
    fwrite(line,1,Bytes_Read, file2);
   }

   fclose(file1);
   fclose(file2);
}

/* Send message to all clients except sender */
void Send_Message(char *text, int thCount, int case_treat,char* username)
{
  pthread_mutex_lock(&mlock);

  for (int i = 1; i < nthreads; ++i)
  {  //server si client
    if (case_treat == 0) {
      if (threadsPool[i] != NULL && threadsPool[i]->thCount == thCount)
        if (send(threadsPool[i]->sd, text, strlen(text), 0) < 0) {
          perror("[server].Eroare la send(). //scrierea descriptorului  \n");
          break;
        }
    } else // Grup
    if (case_treat == 1) {
      if (threadsPool[i] != NULL && threadsPool[i]->Status_Privat_Or_Public ==0 && threadsPool[i]->thCount != thCount)
        {
          if (send(threadsPool[i]->sd, text, strlen(text), 0) < 0) {
          perror("[server].Eroare la send(). //scrierea descriptorului  \n");
          break;
          }
        }
    }else  //prieten
       if (case_treat == 2) {
      if (threadsPool[i] != NULL && threadsPool[i]->Status_Privat_Or_Public ==1 && threadsPool[i]->thCount == thCount)
          if (send(threadsPool[i]->sd, text, strlen(text), 0) < 0) {
          perror("[server].Eroare la send(). //scrierea descriptorului  \n");
          break;
        }
    }
  }

  pthread_mutex_unlock(&mlock);
}

void Show_Offline_Messages(char* username, thData *Client)
{
  FILE* file = fopen("Offline_Messages.txt", "r+");
  if (file == NULL)
    printf("[Server]Eroare la deschiderea fisierului Offline_Messages.");

  char line[4000];
  char copy_username[Length_Username];
  strcpy(copy_username,username);
  strcat(copy_username,":");

  FILE* file2=fopen("Copy_Offlinr_Messages.txt","w+");
  if(file2==NULL)
   printf("[Server]Eroare la deschiderea fisierului Offline_Messages.");

  while (fgets(line, sizeof(line), file) != NULL)
  {
    char sender[Length_Username];
    char recipient[Length_Username];
    sscanf(line, "%s -> %s", sender, recipient);

    if (strcmp(recipient, copy_username) == 0)
    {
      Add_History_Message(username, line, 1);
      Send_Message(line, Client->thCount, 0,Client->Username);
    }
    else {
    fprintf(file2,"%s", line);
    }
  }

  if(remove("Offline_Messages.txt")!=0)
   printf("[Server]Eroare la stergerea fisierului. \n");

  if(rename("Copy_Offlinr_Messages.txt", "Offline_Messages.txt")!=0)
   printf("[Server]Eroare la redenumirea fisierului. \n");


  fclose(file2);
  fclose(file);
 }

void Uddate_file(int thCount, char* message)
{
   FILE* file = fopen("users_messages_history.txt", "a+");
  if (file == NULL)
    perror("[server]Eroare la deschiderea fisierului users_messages_history.txt. \n");

   char Identificator[25];
   sprintf(Identificator, "Thread-%d", thCount);

   char line[nthreads];
   int ok=0;

   while(fgets(line, sizeof(line), file)!=NULL)
   {
    if(strncmp(line, Identificator, strlen(Identificator))==0)
    {
       ok=1;
       break;
    }
   }

   if(ok)
   {
    fseek(file, 0, SEEK_END);
    fprintf(file, "%s\n", message);
   }
   else
   {
    fprintf(file, "%s\n%s\n", Identificator, message);
   }

   fclose(file);
}


int Check_User_Passward(char *username, char *password) {
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
      if (status_account == 0) {
        ok = 1;
        fseek(file, -3, SEEK_CUR); // ne mutam cu o pozitie inapoi pe 0 ca sa l
                                   // putem suprascrie
        fputc('1', file); // suprascrierea
        fflush(file);     // salvarea modificarilor
        break;
      } else {
        ok = -1;
        break;
      }
    }
  }

  fclose(file);
  return ok;
}

void Log_In(char *user, char *password, int *ok, thData *Client) {
  char Text[Size_Message];
  if (strlen(user) <= 0) {
    printf(" Nu ati introdus Username-ul a\n");
    *ok = 1;
  } else {
    if (strlen(password) <= 0) {
      printf(" Nu ati introdus Parola. /%s\n", password);
      *ok = 1;
    } else {
      int Status_Account = Check_User_Passward(user, password);
      if (Status_Account == 0) {
        bzero(Text, sizeof(Text));
        sprintf(Text, " Autentificare esuata pentru %s. \n", user);
        printf("%s ", Text);
        Send_Message(Text, Client->thCount, 0, Client->Username);
        close(Client->sd);
        Remove_Client(Client->thCount);
        free(Client);
        Count_Clients--;
        pthread_detach(pthread_self());
      } else if (Status_Account == -1) {
        bzero(Text, sizeof(Text));
        sprintf(Text," Autentificare esuata pentru %s. ",user);
        printf("%s ", Text);
        Send_Message(Text, Client->thCount, 0, Client->Username);
        close(Client->sd);
        Remove_Client(Client->thCount);
        free(Client);
        Count_Clients--;
        pthread_detach(pthread_self());
      } else {
        bzero(Text, sizeof(Text));
        strcpy(Client->Password, password);
        strcpy(Client->Username, user);
        Add_History_Message(user, " ", -1);
        sprintf(Text, " Userul %s  s-a conectat cu parola cu succes. \n", Client->Username);
        printf("%s", Text);
        Send_Message(Text, Client->thCount, 0, Client->Username);
      }
    }
  }

  bzero(Text, sizeof(Text));
}

void Log_Out(char *username, char *password, thData *Client)
{
  FILE *file = fopen("users.txt", "r+w");
  if (file == NULL)
    perror("[server]Eroare la deschiderea fisierului users.txt. \n");

  char line[nthreads];

  while (fgets(line, sizeof(line), file)) {
    char candidate_password[Size_Password];
    char candidate_username[Length_Username];
    int status_account;
    sscanf(line, "%s %s %d", candidate_username, candidate_password,
           &status_account);
    if (strcmp(candidate_username, username) == 0 &&
        strcmp(candidate_password, password) == 0) {
      if (status_account == 1) {
        fseek(file, -3, SEEK_CUR); // ne mutam cu 2 pozitii 0+endl inapoi pe 1
                                   // ca sa l putem suprascrie
        fputc('0', file); // suprascrierea
        fflush(file);     // salvarea modificarilor
        break;
      }
    }
  }
  fclose(file);

  char Copy[Size_Message];
  sprintf(Copy, "S-a deconectat (%s). :( \n", username);
  printf(" %s", Copy);

  /* Stergem si eliberam memoria alocata thredului unui client*/
  close(Client->sd);
  Remove_Client(Client->thCount);
  free(Client);
  Count_Clients--;
  pthread_detach(pthread_self());
}

void Reply(char* Text, thData* Client, int n)
{
  Delet_endl(Text, strlen(Text));
        char* From_Who=strtok(Text,":");
        char* Command_Reply=strtok(NULL,":");
        char* Question=strtok(NULL,":");
        char* Answer=strtok(NULL,":");

        From_Who[0]='\0';
        Command_Reply[0]='\0';
        char Reply_Message[Size_Message];
        sprintf(Reply_Message, "Raspunsul pentru %s este %s.\n", Question,Answer);
        printf("-> %s ", Reply_Message);
        Send_Message(Reply_Message, Client->thCount, n,  Client->Username);
        Add_History_Message(Client->Username, Reply_Message, 1);

}

/* Gestioneaza comenzile  */
void *Treat_Client(void *arg) {
  char User[Length_Username];
  char Password[Length_Username];
  char Grupa_Or_Privet[Length_Username];
  int ok = 0;

  bzero(Password, sizeof(Password));
  bzero(User, sizeof(User));
  Count_Clients++;
  thData *Client = (thData *)arg;

  // Primirea numelui de la client
  recv(Client->sd, User, Length_Username, 0);
  Delet_endl(User, strlen(User));
  recv(Client->sd, Password, Size_Password, 0);
  Delet_endl(Password, strlen(Password));
// Decriptare parola dupa primul caracter al userului
  Log_In(User, Password, &ok, Client);
  recv(Client->sd, Grupa_Or_Privet, Size_Password, 0);
  to_lower(Grupa_Or_Privet);
  Delet_endl(Grupa_Or_Privet, strlen(Password));

  // afla thCount ul pe care il are prietenul
  if(strlen(Client->Prieten)>0)
  {
    Client->thCount_Prieten=-1;

    for(int i=1; i<nthreads; i++)
    if(threadsPool[i]!=NULL && strcmp(threadsPool[i]->Username, Client->Prieten)==0)
    {
      Client->thCount_Prieten=threadsPool[i]->thCount;
      threadsPool[i]->thCount_Prieten=Client->thCount;
      break;
    }
  }

  if(strstr(Grupa_Or_Privet, "grup")!=NULL)
    {
      Grup=1;
      Client->Status_Privat_Or_Public=0;
      Add_History_Message(Client->Username, " Grup: \n", 0);
      Show_Offline_Messages(Client->Username,Client);
    }
  else
    if(strstr(Grupa_Or_Privet, "privat")!=NULL)
    {
      recv(Client->sd, Client->Prieten, Length_Prieten, 0);
      Delet_endl(Client->Prieten, strlen(Client->Prieten));
      Privat=1;
      Client->Status_Privat_Or_Public=1;
      Add_History_Message(Client->Username, " Privat: \n", 0);
      Show_Offline_Messages(Client->Username, Client);
    }

  char Text[Size_Message];
  
  while (1) {
    if (ok)
      break;

    recv(Client->sd, Text, Size_Message, 0);

    if (strlen(Text) != 0) {
      if (strstr(Text, "decon") != NULL || strstr(Text, "exit") != NULL ) {
        ok = 1;
        Log_Out(Client->Username, Client->Password, Client);
        break;
      }
      else
      if(Grup==1)
      {
         if(strstr(Text,"reply")!=NULL || strstr(Text,"Reply")!=NULL)
         {
         Reply(Text,Client,1);
         }
         else
         {
        Send_Message(Text, Client->thCount, 1,  Client->Username);
        Delet_endl(Text, strlen(Text));
        printf("-> %s \n", Text);
        Add_History_Message(Client->Username, Text, 1);
         }
      }
      else
      if(Privat==1)
      {

         if(strstr(Text,"reply")!=NULL || strstr(Text,"Reply")!=NULL)
       {
        char* From_Who=strtok(Text,":");
        char* Command_Reply=strtok(NULL,":");
        char* Question=strtok(NULL,":");
        char* Answer=strtok(NULL,":");

        From_Who[0]='\0';
        Command_Reply[0]='\0';
        char Reply_Message[Size_Message];
       Delet_endl(Answer, strlen(Answer));
        sprintf(Reply_Message, "Raspunsul pentru %s este %s.", Question,Answer);
        printf("-> %s \n" , Reply_Message);
       
        Send_Message(Reply_Message, Client->thCount_Prieten, 2,Client->Username);
        Add_History_Message(Client->Username, Reply_Message, 1);
        if(Online_Offline_User(Client->Prieten)==0)
        {
           Save_Offline_message(Client->Username, Client->Prieten, Text);
           printf("%s este offline\n", Client->Prieten);
        }
       }
         else
        {
        printf("-> %s \n" , Text);
        Delet_endl(Text, strlen(Text));
        if(Online_Offline_User(Client->Prieten)==0)
        {
           Save_Offline_message(Client->Username, Client->Prieten, Text);
           printf("%s este offline\n", Client->Prieten);
        }
        else {
        Send_Message(Text, Client->thCount_Prieten, 2, Client->Username);
        Add_History_Message(Client->Username, Text, 1);
        }
        }
     }
    }
    bzero(Text, sizeof(Text));
  
     if(strlen(Client->Prieten)>0)
      {
        Client->thCount_Prieten=-1;

        for(int i=1; i<nthreads; i++)
        if(threadsPool[i]!=NULL && strcmp(threadsPool[i]->Username, Client->Prieten)==0)
        {
          Client->thCount_Prieten=threadsPool[i]->thCount;
          threadsPool[i]->thCount_Prieten=Client->thCount;
          break;
        }
      }
  }

  return 0;
}

int check(int result, const char *error_message) {
  if (result < 0) {
    perror(error_message);
    return (errno);
  }

  return 0;
}

/* programul */
int main(int argc, char *argv[]) {
  struct sockaddr_in Server; /* structurile pentru server si clienti 0 protocolul pentur transport*/
  struct sockaddr_in Client;
  int optval = 1; /* optiune folosita pentru setsockopt()*/
  int fd_server = 0; /* descriptor folosit pentru  parcurgerea listelor de descriptori */
  int fd_client = 0;
  pthread_t ID_New_Thread;
  Set_All_Users_Offline();

  if (argc != 3) {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  PORT = atoi(argv[2]);

  /* crearea unui socket */
  fd_server = socket(AF_INET, SOCK_STREAM, 0);
  Server.sin_family = AF_INET;
  Server.sin_addr.s_addr = inet_addr(argv[1]);
  Server.sin_port = htons(PORT);

  /*setam pentru socket optiunea SO_REUSEADDR */
  check(setsockopt(fd_server, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR),
                   (char *)&optval, sizeof(optval)),
        "[server]Eroare la setsocketopt(). \n");

  /* atasam IP si Portul cu socketul */
  check(bind(fd_server, (struct sockaddr *)&Server, sizeof(Server)),
        "[server]Eroare la bind(). \n");

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen(fd_server, 5) < -1) {
    perror("[server]Eroare la listen().\n");
    return errno;
  }

  printf("<------------> WELCOME TO LINUX MESSENGER <------------> \n\n");

  while (1)
  {
    socklen_t clilen = sizeof(Client);
    fd_client = accept(fd_server, (struct sockaddr *)&Client, &clilen);

    /* Verificam daca am atins capacitatea maxima a servarului*/
    Count_Clients++;
    if (Count_Clients >= nthreads) {
      printf("[server]Limita maxima a clientilor a fost atinsa: %d. Userul cu Portul: %s si adresa servarului %d este respins. \n",
             nthreads, inet_ntoa(Client.sin_addr), ntohs(Client.sin_port));
      close(fd_client);
      continue;
    }

    /* Informatiile clientului */
    thData *Client_Information = (thData *)malloc(sizeof(thData));
    Client_Information->sd = fd_client;
    Client_Information->thCount = thCount++;

    /* Adaugam un nou client si thredul asociat atr*/
    Add_Clinet(Client_Information);
    pthread_create(&ID_New_Thread, NULL, &Treat_Client,
                   (void *)Client_Information);

    sleep(1);
  } /* while */

  return 1;
} /* main */
