#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sqlite3.h>

#define PORT 3000
#define MAX_PRODUSE 100
#define MAX_ISTORIC 100
#define MAX_UTILIZATORI 100
#define MAX_COS 10

typedef struct
{
    int id_produs;
    char nume_produs[50];
    char producator[50];
    char contact[50];
    float pret;
} Produs;

typedef struct
{
    int logat;
    char nume_utilizator[50];
    char parola[50];
} Utilizator;

typedef struct
{
    char nume[50];
    char prenume[50];
    char adresa[50];
    char nr_telefon[50];
    int id_utilizator;
} InformatiiUtilizator;

Produs produse[MAX_PRODUSE];
Utilizator utilizatori[MAX_UTILIZATORI];
Produs produseDisponibile[MAX_PRODUSE];
int numarProduse = 0;
int numarAchizitii = 0;
int numarUtilizatori = 0;

enum OptiuniClient
{
    vizualizare_produse,           // Vizualizare produse disponibile
    adauga_in_cos,                 // Adăugare produs în coș de cumpărături
    vizualizare_istoric_achizitii, // Vizualizare istoricul achizițiilor
    adauga_produse_vanzare,        // Adăugare produse pentru vânzare
    cumpara_produse,               // Cumpărare produse
    deconectare,                   // Alerțe și notificări
    login,                         // login
    setari_cont,                   // Setări cont
    retur,                         // Returnare produse
    recenzii,                      // Scriere recenzii
    iesire                         // Ieșire
};

int main()
{
    int sd;
    struct sockaddr_in server;
    char optiune[50];

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[client]Eroare la crearea socketului.\n");
        return errno;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) <= 0)
    {
        perror("[client]Eroare la inet_pton().\n");
        return errno;
    }

    server.sin_port = htons(PORT);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Eroare la conectarea la server.\n");
        return errno;
    }
    char cosProduse[MAX_COS][100];
    int numarProduseCos = 0;
    int logat = 0; // 0 = nelogat, 1 = logat
    do
    {
        // Afiseaza meniul
        printf("\n");
        printf("iesire\n");                        // rezolvat
        printf("vizualizare_produse\n");           // rezolvat
        printf("adauga_in_cos\n");                 // rezolvat
        printf("vizualizare_istoric_achizitii\n"); // rezolvat
        printf("adauga_produse_vanzare\n");        // rezolvat
        printf("cumpara_produse\n");               // rezolvat
        printf("deconectare\n");                   // rezolvat
        printf("login\n");                         // rezolvat
        printf("setari_cont\n");                   // rezolvat
        printf("retur\n");                         // rezolvat
        printf("recenzii\n");                      // rezolvat
        printf("\n");
        printf("Alegeti o optiune: ");
        scanf("%s", optiune);
        printf("\n");

        if (write(sd, optiune, sizeof(optiune)) <= 0)
        {
            perror("[client]Eroare la scrierea catre server.\n");
            return errno;
        }

        int ok = 0; // variabila pt a decide daca o comanda este valida

        // Exemplu pentru citirea datelor de la server în funcția de vizualizare a produselor
        if (strcmp(optiune, "vizualizare_produse") == 0)
        {
            ok = 1;
            int numarProduseServer;
            if (read(sd, &numarProduseServer, sizeof(int)) <= 0)
            {
                perror("[client]Eroare la citirea de la server.\n");
                return errno;
            }

            Produs produseServer[MAX_PRODUSE];
            if (read(sd, produseServer, sizeof(Produs) * numarProduseServer) <= 0)
            {
                perror("[client]Eroare la citirea de la server.\n");
                return errno;
            }

            printf("Produse disponibile:\n");
            for (int i = 0; i < numarProduseServer; ++i)
            {
                printf("%d %s %s %s %.2f lei\n", produseServer[i].id_produs, produseServer[i].nume_produs, produseServer[i].producator, produseServer[i].contact, produseServer[i].pret);
            }
        }

        if (strcmp(optiune, "adauga_in_cos") == 0)
        {
            ok = 1;
            if (logat)
            {
                // Afișează lista de produse în cos (dacă există)
                if (read(sd, &numarProduseCos, sizeof(int)) <= 0)
                {
                    perror("[client] Eroare la citirea numărului de produse din cos.\n");
                    return errno;
                }

                if (numarProduseCos > 0)
                {
                    // Citeste lista de produse din cos
                    if (read(sd, cosProduse, sizeof(cosProduse)) <= 0)
                    {
                        perror("[client] Eroare la citirea listei de produse din cos.\n");
                        return errno;
                    }

                    printf("Produse in cos:\n");
                    for (int i = 0; i < numarProduseCos; ++i)
                    {
                        printf("%s\n", cosProduse[i]);
                    }
                }
                else
                {
                    printf("Nu ai produse in cos.\n");
                }
                // Solicitarea numelui produsului pentru adăugare în coș
                char numeProdusCos[50];
                printf("Introduceti numele produsului pe care doriti sa-l adaugati in cos: ");
                scanf("%s", numeProdusCos);

                // Trimite serverului numele produsului pentru adaugare in cos
                if (write(sd, numeProdusCos, sizeof(numeProdusCos)) <= 0)
                {
                    perror("[client]Eroare la scrierea numelui produsului pentru adaugare in cos.\n");
                    return errno;
                }

                // Primește răspunsul de la server (produs adaugat sau nu)
                char mesaj[100];
                if (read(sd, mesaj, sizeof(mesaj)) <= 0)
                {
                    perror("[client]Eroare la citirea raspunsului pentru adaugare in cos.\n");
                    return errno;
                }

                if (strcmp(mesaj, "Produs adaugat cu succes in cos.") == 0)
                    numarProduseCos++;

                printf("%s\n", mesaj);
            }
            else
            {
                printf("Trebuie să vă autentificați pentru a adăuga produse in cos.\n");
            }
        }

        if (strcmp(optiune, "vizualizare_istoric_achizitii") == 0)
        {
            ok = 1;
            if (logat)
            {
                char produse_lista[(MAX_COS + 1) * 100];
                read(sd, produse_lista, sizeof(produse_lista));

                printf("%s\n", produse_lista);
            }
            else
            {
                printf("Trebuie să vă autentificați pentru a vizaliza istoricul achizitiilor.\n");
            }
        }

        if (strcmp(optiune, "adauga_produse_vanzare") == 0)
        {
            ok = 1;
            if (logat)
            {
                Produs produsNou;
                printf("Introduceti id-ul produsului: ");
                scanf("%d", &produsNou.id_produs);
                printf("Introduceti numele produsului: ");
                scanf("%s", produsNou.nume_produs);
                printf("Introduceti producatorul produsului: ");
                scanf("%s", produsNou.producator);
                printf("Introduceti contact: ");
                scanf("%s", produsNou.contact);
                printf("Introduceti pretul produsului: ");
                scanf("%f", &produsNou.pret);

                // Adaugă produsul local în vectorul local produse
                if (numarProduse < MAX_PRODUSE)
                {
                    produse[numarProduse] = produsNou;
                    numarProduse++;
                }

                // Trimite comanda către server pentru a actualiza baza de date
                if (write(sd, &produsNou, sizeof(Produs)) <= 0)
                {
                    perror("[client]Eroare la scrierea catre server.\n");
                    return errno;
                }
            }
            else
            {
                printf("Trebuie să vă autentificați pentru a adauga produse la vanzare.\n");
            }
        }

        // Inside the section where you handle the "cumpara_produse" option
        if (strcmp(optiune, "cumpara_produse") == 0)
        {
            ok = 1;
            if (logat)
            {
                // Receive the purchase message from the server
                char purchaseMessage[100];
                if (read(sd, purchaseMessage, sizeof(purchaseMessage)) <= 0)
                {
                    perror("[client] Eroare la citirea comenzii cumpara_produse.\n");
                    return errno;
                }

                printf("%s\n", purchaseMessage);

                if (numarProduseCos == 0)
                {
                    printf("NU ai produse disponibile in cos.\n");
                }
                else
                {
                    // Process each product in the shopping cart
                    for (int i = 0; i < numarProduseCos; ++i)
                    {
                        // Receive product details from the server
                        char productDetails[100];
                        if (read(sd, productDetails, sizeof(productDetails)) <= 0)
                        {
                            perror("[client] Eroare la citirea detaliilor produsului de la server.\n");
                            return errno;
                        }

                        // Display the product details and prompt for quantity
                        printf("%s introduceti cantitatea: ", productDetails);

                        int quantity;
                        scanf("%d", &quantity);

                        // Send the quantity to the server
                        if (write(sd, &quantity, sizeof(int)) <= 0)
                        {
                            perror("[client] Eroare la scrierea cantitatii catre server.\n");
                            return errno;
                        }
                    }
                    float totalSum;
                    if (read(sd, &totalSum, sizeof(float)) <= 0)
                    {
                        perror("[server] Eroare la citirea pretului total de la server.\n");
                        return errno;
                    }
                    printf("PRET TOTAL: %f\n\n", totalSum);

                    // Prompt for purchase confirmation
                    char confirmationPrompt[15];
                    if (read(sd, confirmationPrompt, sizeof(confirmationPrompt)) <= 0)
                    {
                        perror("[client] Eroare la citirea promptului de confirmare de la server.\n");
                        return errno;
                    }

                    printf("%s ", confirmationPrompt);

                    // Get user response
                    char response[3];
                    scanf("%s", response);

                    // Send the response to the server
                    if (write(sd, response, sizeof(response)) <= 0)
                    {
                        perror("[client] Eroare la scrierea raspunsului catre server.\n");
                        return errno;
                    }

                    char Message[23];
                    if (read(sd, Message, sizeof(Message)) <= 0)
                    {
                        perror("[server] Eroare la trimiterea mesajului de achizitionare reusita catre client.\n");
                        return errno;
                    }
                    printf("%s\n", Message);
                }
            }
            else
            {
                printf("Trebuie să vă autentificați pentru a cumpăra produse.\n");
            }
        }

        if (strcmp(optiune, "deconectare") == 0)
        {
            ok = 1;
            if (logat)
            {
                logat = 0; // Deconectare utilizator
                printf("V-ati deconectat cu succes.\n");
                // ... alte acțiuni de deconectare, dacă este cazul
            }
            else
            {
                printf("Trebuie să vă autentificați pentru a vă deconecta.\n");
            }
        }

        if (strcmp(optiune, "login") == 0)
        {
            ok = 1;

            if (!logat) // Permite doar utilizatorilor nelogati să efectueze login
            {
                // Solicitarea numelui de utilizator
                char utilizator[50];
                logat = 0;
                printf("Introduceti nume utilizator: ");
                scanf("%s", utilizator);

                // Trimite serverului numele de utilizator
                if (write(sd, utilizator, sizeof(utilizator)) <= 0)
                {
                    perror("[client]Eroare la scrierea numelui de utilizator.\n");
                    return errno;
                }

                // Solicitarea parolei
                char parola[50];
                printf("Introduceti parola: ");
                scanf("%s", parola);

                // Trimite serverului parola
                if (write(sd, parola, sizeof(parola)) <= 0)
                {
                    perror("[client]Eroare la scrierea parolei.\n");
                    return errno;
                }

                parola[strcspn(parola, "\n")] = '\0';
                // Așteaptă răspunsul de la server (daca login-ul a fost sau nu reușit)
                int loginStatus;
                if (read(sd, &loginStatus, sizeof(int)) <= 0)
                {
                    perror("[client]Eroare la citirea statusului login-ului.\n");
                    return errno;
                }

                if (loginStatus == 1)
                {
                    logat = 1; // Actualizează starea de conectare
                    printf("Login reusit!\n");
                }
                else
                {

                    printf("Login esuat. Numele de utilizator sau parola incorecta.\n");
                }
            }
            else
            {
                char raspuns[28];
                read(sd, raspuns, sizeof(raspuns));

                printf("%s\n", raspuns);
            }
        }
        if (strcmp(optiune, "setari_cont") == 0)
        {
            ok = 1;
            if (logat)
            {
                InformatiiUtilizator infoUtilizator;

                // Receive user information from the server
                if (read(sd, &infoUtilizator, sizeof(InformatiiUtilizator)) <= 0)
                {
                    perror("[client]Eroare la citirea informatiilor despre utilizator.\n");
                    return errno;
                }

                // Display user information
                printf("Nume: %s\n", infoUtilizator.nume);
                printf("Prenume: %s\n", infoUtilizator.prenume);
                printf("Adresa: %s\n", infoUtilizator.adresa);
                printf("Nr. telefon: %s\n", infoUtilizator.nr_telefon);
                printf("ID Utilizator: %d\n", infoUtilizator.id_utilizator);
            }
            else
            {
                printf("Trebuie să vă autentificați pentru a accesa setările contului.\n");
            }
        }

        if (strcmp(optiune, "retur") == 0)
        {
            ok = 1;
            if (logat)
            {

                char numeProdusRetur[50];
                printf("Introduceti numele produsului pe care doriti sa-l returnati: ");
                scanf("%s", numeProdusRetur);

                if (write(sd, numeProdusRetur, sizeof(numeProdusRetur)) <= 0)
                {
                    perror("[client]Eroare la scrierea numelui produsului pentru retur.\n");
                    return errno;
                }

                char returStatus[100];
                if (read(sd, returStatus, sizeof(returStatus)) <= 0)
                {
                    perror("[client]Eroare la citirea statusului returului de la server.\n");
                    return errno;
                }

                printf("%s\n", returStatus);
            }
            else
            {
                printf("Trebuie să vă autentificați pentru a returna un produs.\n");
            }
        }
        if (strcmp(optiune, "recenzii") == 0)
        {
            ok = 1;
            // Solicită numele produsului pentru a afișa recenziile
            char numeProdusRecenzie[50];
            printf("Introduceti numele produsului pentru recenzii: ");
            scanf("%s", numeProdusRecenzie);

            // Trimite serverului numele produsului pentru recenzii
            if (write(sd, numeProdusRecenzie, sizeof(numeProdusRecenzie)) <= 0)
            {
                perror("[client] Eroare la scrierea numelui produsului pentru recenzii.\n");
                return errno;
            }

            // Primește și afișează recenziile de la server
            int recenziiLength;
            if (read(sd, &recenziiLength, sizeof(recenziiLength)) <= 0)
            {
                perror("[client] Eroare la citirea lungimii recenziilor de la server.\n");
                return errno;
            }

            if (recenziiLength > 0)
            {
                char recenziiProdus[recenziiLength + 1];
                recenziiProdus[recenziiLength] = '\0';

                ssize_t bytesRead = read(sd, recenziiProdus, recenziiLength);
                if (bytesRead <= 0)
                {
                    perror("[client] Eroare la citirea recenziilor de la server.\n");
                    return errno;
                }

                // Afișează recenziile normale
                printf("Recenzii pentru produsul %s:\n%s\n", numeProdusRecenzie, recenziiProdus);
            }
            else
            {
                // Afișează mesajul 'fara recenzii'
                char noReviews[50];
                if (read(sd, noReviews, sizeof(noReviews)) <= 0)
                {
                    perror("[client] Eroare la citirea mesajului 'fara recenzii' de la server.\n");
                    return errno;
                }
                printf("%s\n", noReviews);
            }
        }
        if (strcmp(optiune, "iesire") == 0)
        {
            ok = 1;
            break; // Ieși din buclă dacă utilizatorul a introdus "iesire"
        }
        if (ok == 0)
        {
            printf("Comanda invalida! Va rugam introduceti o comanda valida.\n");
        }

    } while (strcmp(optiune, "iesire") != 0);

    close(sd);

    return 0;
}
