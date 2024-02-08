#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
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
    char utilizator[50];
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

typedef struct
{
    int logat;
    int idThread;
    int cl;
    Utilizator utilizatorCurent;
} ThData;

typedef struct
{
    char produs_nume[MAX_COS];
    float pret;
} ProdusCos;

Produs produse[MAX_PRODUSE];
Utilizator utilizatori[MAX_UTILIZATORI];
Produs produseDisponibile[MAX_PRODUSE];
int numarProduse = 0;
int numarAchizitii = 0;
int numarUtilizatori = 0;

pthread_mutex_t lacat = PTHREAD_MUTEX_INITIALIZER;

enum OptiuniClient
{
    vizualizare_produse,           // Vizualizare produse disponibile
    adauga_in_cos,                 // Adăugare produs în coș de cumpărături
    vizualizare_istoric_achizitii, // Vizualizare istoricul achizițiilor
    adauga_produse_vanzare,        // Adăugare produse pentru vânzare
    cumpara_produse,               // Cumpărare produse
    alerte_si_notificari,          // Alerțe și notificări
    login,                         // login
    setari_cont,                   // Setări cont
    retur,                         // Returnare produse
    recenzii,                      // Scriere recenzii
    iesire                         // Ieșire
};

void initializareProduse()
{
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("/home/garbiacosmin/Desktop/baza_de_date_proiect_RC", &db) != SQLITE_OK)
    {
        fprintf(stderr, "Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *query = "SELECT id_produs, nume_produs, producator, contact, pret FROM PRODUSE;";
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "Eroare la pregătirea interogării: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    numarProduse = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW && numarProduse < MAX_PRODUSE)
    {
        produse[numarProduse].id_produs = sqlite3_column_int(stmt, 0);
        strcpy(produse[numarProduse].nume_produs, (const char *)sqlite3_column_text(stmt, 1));
        strcpy(produse[numarProduse].producator, (const char *)sqlite3_column_text(stmt, 2));
        strcpy(produse[numarProduse].contact, (const char *)sqlite3_column_text(stmt, 3));
        produse[numarProduse].pret = sqlite3_column_double(stmt, 4);
        numarProduse++;
    }

    sqlite3_finalize(stmt);

    // Citirea datelor despre utilizatori
    const char *queryUtilizatori = "SELECT utilizator, parola FROM CLIENTI;";
    if (sqlite3_prepare_v2(db, queryUtilizatori, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "Eroare la pregătirea interogării pentru utilizatori: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    numarUtilizatori = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW && numarUtilizatori < MAX_UTILIZATORI)
    {
        strcpy(utilizatori[numarUtilizatori].utilizator, (const char *)sqlite3_column_text(stmt, 0));
        strcpy(utilizatori[numarUtilizatori].parola, (const char *)sqlite3_column_text(stmt, 1));
        utilizatori[numarUtilizatori].logat = 0; // Setează logat la 0 pentru noul utilizator
        numarUtilizatori++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void gestioneazaCerereClient(ThData *tdL)
{
    ProdusCos produse_cos[MAX_PRODUSE];
    int logat = 0;
    int numarProduseCos = 0;
    int numarProduseAchizitionate = 0;
    float achizitieSum = 0;
    sqlite3 *db;
    if (sqlite3_open("/home/garbiacosmin/Desktop/baza_de_date_proiect_RC", &db) != SQLITE_OK)
    {
        perror("[Thread]Eroare la deschiderea bazei de date.\n");
        return;
    }

    char optiune[50];
    do
    {
        if (read(tdL->cl, optiune, sizeof(optiune)) <= 0)
        {
            perror("[Thread]Eroare la citirea de la client.\n");
            return;
        }

        if (strcmp(optiune, "iesire") == 0)
        {
            // Ieșire din buclă
            break;
        }

        if (strcmp(optiune, "vizualizare_produse") == 0)
        {
            if (write(tdL->cl, &numarProduse, sizeof(int)) <= 0)
            {
                perror("[Thread]Eroare la scrierea catre client.\n");
                return;
            }

            if (write(tdL->cl, produse, sizeof(Produs) * numarProduse) <= 0)
            {
                perror("[Thread]Eroare la scrierea catre client.\n");
                return;
            }
        }

        if (strcmp(optiune, "adauga_in_cos") == 0)
        {
            if (logat)
            {

                if (write(tdL->cl, &numarProduseCos, sizeof(int)) <= 0)
                {
                    perror("[server] Eroare la scrierea numarului de produse din cos catre client.\n");
                    return;
                }

                if (numarProduseCos > 0)
                {
                    char produse_lista[MAX_COS][100];

                    for (int i = 0; i < numarProduseCos; ++i)
                    {
                        strcpy(produse_lista[i], produse_cos[i].produs_nume);
                    }

                    if (write(tdL->cl, produse_lista, sizeof(produse_lista)) <= 0)
                    {
                        perror("[server] Eroare la scrierea listei de produse din cos catre client.\n");
                        return;
                    }
                }

                char numeProdusAdaugat[50];
                if (read(tdL->cl, numeProdusAdaugat, sizeof(numeProdusAdaugat)) <= 0)
                {
                    perror("[server] Eroare la citirea numelui produsului pentru adaugare in cos.\n");
                    return;
                }

                // Verifică dacă produsul există în baza de date
                int produsExista = 0;
                int iterator_produse;
                for (iterator_produse = 0; iterator_produse < numarProduse; ++iterator_produse)
                {
                    if (strcmp(produse[iterator_produse].nume_produs, numeProdusAdaugat) == 0)
                    {
                        produsExista = 1;
                        break;
                    }
                }

                // Răspunde clientului în funcție de rezultatul verificării
                if (produsExista)
                {
                    // Adaugă produsul în coș
                    if (numarProduseCos < MAX_COS)
                    {
                        strcpy(produse_cos[numarProduseCos].produs_nume, numeProdusAdaugat);
                        produse_cos[numarProduseCos].pret = produse[iterator_produse].pret;
                        numarProduseCos++;
                    }

                    // Trimite mesaj de succes către client
                    char succesMesaj[] = "Produs adaugat cu succes in cos.";
                    if (write(tdL->cl, succesMesaj, sizeof(succesMesaj)) <= 0)
                    {
                        perror("[server] Eroare la scrierea mesajului de succes pentru adaugare in cos.\n");
                        return;
                    }
                }
                else
                {
                    // Trimite mesaj că produsul nu există în baza de date
                    char eroareMesaj[] = "Produsul nu exista in baza de date.";
                    if (write(tdL->cl, eroareMesaj, sizeof(eroareMesaj)) <= 0)
                    {
                        perror("[server] Eroare la scrierea mesajului de eroare pentru adaugare in cos.\n");
                        return;
                    }
                }
            }
            else
            {
                printf("Trebuie să vă autentificați pentru a adauga produse in cos.\n");
            }
        }

        if (strcmp(optiune, "vizualizare_istoric_achizitii") == 0)
        {
            if (logat)
            {
                char achizitie[(MAX_COS + 1) * 100] = "";

                int i;
                for (i = 0; i < numarProduseAchizitionate; ++i)
                {
                    strcat(achizitie, produse_cos[i].produs_nume);
                    strcat(achizitie, "\n");
                }

                strcat(achizitie, "PRET_TOTAL: ");

                char float_char[318] = "";
                sprintf(float_char, "%f", achizitieSum);
                strcat(achizitie, float_char);

                printf("%s\n", achizitie);
                write(tdL->cl, achizitie, sizeof(achizitie));
            }
            else
            {
                write(tdL->cl, "Trebuie să vă autentificați pentru a vizualiza istoricul.", sizeof("Trebuie să vă autentificați pentru a vizualiza istoricul."));
            }
        }

        if (strcmp(optiune, "adauga_produse_vanzare") == 0)
        {
            if (logat)
            {
                Produs produsNou;
                if (read(tdL->cl, &produsNou, sizeof(Produs)) <= 0)
                {
                    perror("[Thread]Eroare la citirea de la client.\n");
                    return;
                }
                pthread_mutex_lock(&lacat);

                // Adaugă produsul în baza de date
                char sql[500];
                snprintf(sql, sizeof(sql), "INSERT INTO PRODUSE (id_produs, nume_produs, producator, contact, pret) VALUES (%d, '%s', '%s', '%s', %f);",
                         produsNou.id_produs, produsNou.nume_produs, produsNou.producator, produsNou.contact, produsNou.pret);

                if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
                {
                    perror("[Thread]Eroare la adaugarea produsului in baza de date.\n");
                    write(tdL->cl, "Eroare la adaugarea produsului in baza de date.", sizeof("Eroare la adaugarea produsului in baza de date."));
                }
                else
                {
                    // Dacă adăugarea în baza de date a fost cu succes, actualizează și vectorul local de produse
                    if (numarProduse < MAX_PRODUSE)
                    {
                        produse[numarProduse] = produsNou;
                        numarProduse++;
                        write(tdL->cl, "Produs adaugat cu succes.", sizeof("Produs adaugat cu succes."));
                    }
                    else
                    {
                        write(tdL->cl, "Lista de produse este plina.", sizeof("Lista de produse este plina."));
                    }
                }

                pthread_mutex_unlock(&lacat);
            }
            else
            {
                write(tdL->cl, "Trebuie să vă autentificați pentru a adauga produse la vanzare.", sizeof("Trebuie să vă autentificați pentru a adauga produse la vanzare."));
            }
        }

        if (strcmp(optiune, "cumpara_produse") == 0)
        {
            if (logat)
            {
                // Send a message to the client to start purchasing
                char purchaseMessage[] = "Cumparati produsele din cosul de cumparaturi";
                if (write(tdL->cl, purchaseMessage, sizeof(purchaseMessage)) <= 0)
                {
                    perror("[server] Eroare la scrierea comenzii cumpara_produse.\n");
                    return;
                }

                float totalSum = 0;
                for (int i = 0; i < numarProduseCos; ++i)
                {
                    // Send product details to the client
                    if (write(tdL->cl, produse_cos[i].produs_nume, sizeof(produse_cos[i].produs_nume)) <= 0)
                    {
                        perror("[server] Eroare la trimiterea detaliilor produsului catre client.\n");
                        return;
                    }

                    // Wait for the client to provide the quantity
                    int quantity;
                    if (read(tdL->cl, &quantity, sizeof(int)) <= 0)
                    {
                        perror("[server] Eroare la citirea cantitatii de la client.\n");
                        return;
                    }
                    totalSum = totalSum + quantity * produse_cos[i].pret;
                    achizitieSum = totalSum;
                }
                if (write(tdL->cl, &totalSum, sizeof(float)) <= 0)
                {
                    perror("[server] Eroare la trimiterea pretului total catre client.\n");
                    return;
                }

                // Ask the client if they want to make the purchase
                char prompt[] = "Cumpara? da/nu";
                if (write(tdL->cl, prompt, sizeof(prompt)) <= 0)
                {
                    perror("[server] Eroare la trimiterea promptului catre client.\n");
                    return;
                }

                // Receive the client's response
                char response[3];
                if (read(tdL->cl, response, sizeof(response)) <= 0)
                {
                    perror("[server] Eroare la citirea raspunsului de la client.\n");
                    return;
                }

                if (strcmp(response, "da") == 0)
                {
                    // Successful purchase
                    char successMessage[] = "Achizitionare reusita!";
                    if (write(tdL->cl, successMessage, sizeof(successMessage)) <= 0)
                    {
                        perror("[server] Eroare la trimiterea mesajului de achizitionare reusita catre client.\n");
                        return;
                    }

                    numarProduseAchizitionate += numarProduseCos;

                    numarProduseCos = 0;
                }
                if (strcmp(response, "nu") == 0)
                {
                    // Successful purchase
                    char failureMessage[] = "Achizitionare respinsa";
                    if (write(tdL->cl, failureMessage, sizeof(failureMessage)) <= 0)
                    {
                        perror("[server] Eroare la trimiterea mesajului de achizitionare reusita catre client.\n");
                        return;
                    }
                    for (int i = 0; i < numarProduseCos; ++i)
                    {

                        strcpy(produse_cos[i].produs_nume, "");
                    }
                    numarProduseCos = 0;
                    achizitieSum=0;
                }
            }
            else
            {
                write(tdL->cl, "Trebuie să vă autentificați pentru a cumpara produse.", sizeof("Trebuie să vă autentificați pentru a cumpara produse."));
            }
        }

        if (strcmp(optiune, "deconectare") == 0)
        {
            if (logat)
            {
                logat = 0; // Deconectare utilizator
                printf("[Thread %d] Utilizatorul s-a deconectat.\n", tdL->idThread);
            }
            else
            {
                write(tdL->cl, "Trebuie să vă autentificați pentru a vă deconecta.", sizeof("Trebuie să vă autentificați pentru a vă deconecta."));
            }
        }

        if (strcmp(optiune, "login") == 0)
        {
            if (!logat) // Permite doar utilizatorilor nelogati să efectueze login
            {
                // Citeste numele de utilizator de la client
                char utilizator[50];
                if (read(tdL->cl, utilizator, sizeof(utilizator)) <= 0)
                {
                    perror("[Thread]Eroare la citirea numelui de utilizator.\n");
                    return;
                }

                // Citeste parola de la client
                char parola[50];
                if (read(tdL->cl, parola, sizeof(parola)) <= 0)
                {
                    perror("[Thread]Eroare la citirea parolei.\n");
                    return;
                }

                // Verifica numele de utilizator si parola in baza de date
                int loginStatus = 0; // 0 = esuat, 1 = reusit
                for (int i = 0; i < numarUtilizatori; ++i)
                {
                    if (strcmp(utilizatori[i].utilizator, utilizator) == 0 && strcmp(utilizatori[i].parola, parola) == 0)
                    {
                        loginStatus = 1;
                        tdL->logat = 1;
                        logat = 1; // Actualizează starea de conectare
                        // Adaugă informațiile despre utilizatorul curent în ThData
                        tdL->utilizatorCurent = utilizatori[i];
                        printf("[Thread %d] Utilizatorul %s s-a logat cu succes.\n", tdL->idThread, utilizator);
                        break;
                    }
                }

                // Trimite clientului statusul login-ului
                if (write(tdL->cl, &loginStatus, sizeof(int)) <= 0)
                {
                    perror("[Thread]Eroare la scrierea statusului login-ului.\n");
                    return;
                }
            }
            else
            {
                write(tdL->cl, "Sunteți deja autentificat.", sizeof("Sunteți deja autentificat."));
            }
        }

        if (strcmp(optiune, "setari_cont") == 0)
        {
            if (logat)
            {
                InformatiiUtilizator infoUtilizator;

                // Query database to get user information
                char query[150];
                int i;
                snprintf(query, sizeof(query), "SELECT nume, prenume, adresa, nr_telefon, id_utilizator FROM CLIENTI WHERE utilizator='%s';", tdL->utilizatorCurent.utilizator);

                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK)
                {
                    if (sqlite3_step(stmt) == SQLITE_ROW)
                    {
                        strcpy(infoUtilizator.nume, (const char *)sqlite3_column_text(stmt, 0));
                        strcpy(infoUtilizator.prenume, (const char *)sqlite3_column_text(stmt, 1));
                        strcpy(infoUtilizator.adresa, (const char *)sqlite3_column_text(stmt, 2));
                        strcpy(infoUtilizator.nr_telefon, (const char *)sqlite3_column_text(stmt, 3));
                        infoUtilizator.id_utilizator = sqlite3_column_int(stmt, 4);

                        // Send user information to the client
                        if (write(tdL->cl, &infoUtilizator, sizeof(InformatiiUtilizator)) <= 0)
                        {
                            perror("[Thread]Eroare la scrierea informatiilor despre utilizator.\n");
                            sqlite3_finalize(stmt);
                            return;
                        }
                    }

                    sqlite3_finalize(stmt);
                }
                else
                {
                    perror("[Thread]Eroare la pregatirea interogarii pentru informatii utilizator.\n");
                }
            }
            else
            {
                write(tdL->cl, "Trebuie să vă autentificați pentru a accesa setările contului.", sizeof("Trebuie să vă autentificați pentru a accesa setările contului."));
            }
        }

        if (strcmp(optiune, "retur") == 0)
        {

            if (logat)
            {
                char numeProdusRetur[50];
                if (read(tdL->cl, numeProdusRetur, sizeof(numeProdusRetur)) <= 0)
                {
                    perror("[Thread]Eroare la citirea numelui produsului pentru retur.\n");
                    return;
                }

                pthread_mutex_lock(&lacat);

                int indexProdusRetur = -1;
                for (int i = 0; i < numarProduse; ++i)
                {
                    if (strcmp(produse[i].nume_produs, numeProdusRetur) == 0)
                    {
                        indexProdusRetur = i;
                        break;
                    }
                }

                if (indexProdusRetur != -1)
                {
                    printf("[Thread %d] Retur realizat pentru produsul '%s'. Contactati producatorul: %s\n", tdL->idThread, produse[indexProdusRetur].nume_produs, produse[indexProdusRetur].contact);
                    write(tdL->cl, "Retur realizat cu succes.", sizeof("Retur realizat cu succes."));
                }
                else
                {

                    write(tdL->cl, "Produsul nu a fost gasit.", sizeof("Produsul nu a fost gasit."));
                }

                pthread_mutex_unlock(&lacat);
            }
            else
            {
                write(tdL->cl, "Trebuie să vă autentificați pentru a returna produse.", sizeof("Trebuie să vă autentificați pentru a returna produse."));
            }
        }

        if (strcmp(optiune, "recenzii") == 0)
        {
            char numeProdusRecenzii[50];
            if (read(tdL->cl, numeProdusRecenzii, sizeof(numeProdusRecenzii)) <= 0)
            {
                perror("[Thread] Eroare la citirea numelui produsului pentru recenzii.\n");
                return;
            }

            // Caută în baza de date recenziile pentru produsul specificat
            sqlite3 *db;
            if (sqlite3_open("/home/garbiacosmin/Desktop/baza_de_date_proiect_RC", &db) != SQLITE_OK)
            {
                perror("[Thread] Eroare la deschiderea bazei de date.\n");
                return;
            }

            char query[150];
            snprintf(query, sizeof(query), "SELECT recenzii FROM PRODUSE WHERE nume_produs='%s';", numeProdusRecenzii);

            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK)
            {
                int reviewsSent = 0;

                while (sqlite3_step(stmt) == SQLITE_ROW)
                {
                    const char *recenzie = (const char *)sqlite3_column_text(stmt, 0);

                    if (recenzie != NULL)
                    {
                        // Trimite recenzia către client
                        int recenzieLength = strlen(recenzie);
                        if (write(tdL->cl, &recenzieLength, sizeof(recenzieLength)) <= 0)
                        {
                            perror("[Thread] Eroare la scrierea lungimii recenziei catre client.\n");
                            sqlite3_finalize(stmt);
                            sqlite3_close(db);
                            return;
                        }

                        if (write(tdL->cl, recenzie, recenzieLength) <= 0)
                        {
                            perror("[Thread] Eroare la scrierea recenziei catre client.\n");
                            sqlite3_finalize(stmt);
                            sqlite3_close(db);
                            return;
                        }

                        reviewsSent = 1;
                    }
                }

                sqlite3_finalize(stmt);

                if (!reviewsSent)
                {
                    // Trimite un mesaj special către client dacă nu există recenzii
                    const char *noReviews = "Nu exista recenzii pentru acest produs.";
                    int noReviewsLength = strlen(noReviews);
                    if (write(tdL->cl, &noReviewsLength, sizeof(noReviewsLength)) <= 0)
                    {
                        perror("[Thread] Eroare la scrierea lungimii mesajului 'fara recenzii' catre client.\n");
                    }

                    if (write(tdL->cl, noReviews, noReviewsLength) <= 0)
                    {
                        perror("[Thread] Eroare la scrierea mesajului 'fara recenzii' catre client.\n");
                    }
                }

                sqlite3_close(db);
            }
            else
            {
                perror("[Thread] Eroare la pregatirea interogarii pentru recenzii.\n");
                sqlite3_close(db);
            }
        }

    } while (strcmp(optiune, "iesire") != 0);
    sqlite3_close(db);
    close(tdL->cl);
    pthread_exit(NULL);
}
int main()
{
    int sd;
    struct sockaddr_in server;
    pthread_t thread[100];
    int i = 0;

    initializareProduse(); // Inițializare produse

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la crearea socketului.\n");
        return errno;
    }

    int activare = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &activare, sizeof(activare)) < 0)
    {
        perror("[server]Eroare la activarea optiunilor socketului.\n");
        return errno;
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la legarea adresei la socket.\n");
        return errno;
    }

    if (listen(sd, 2) == -1)
    {
        perror("[server]Eroare la ascultarea pe socket.\n");
        return errno;
    }

    while (1)
    {
        int client;
        ThData *td;
        int lungime = sizeof(struct sockaddr_in);

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        if ((client = accept(sd, (struct sockaddr *)&server, &lungime)) < 0)
        {
            perror("[server]Eroare la acceptarea conexiunii.\n");
            continue;
        }

        td = (ThData *)malloc(sizeof(ThData));
        td->idThread = i++;
        td->cl = client;
        td->logat = 0; // Inițializare la nelogat

        pthread_create(&thread[i], NULL, (void *)gestioneazaCerereClient, td);
        pthread_detach(thread[i]);
    }
    close(sd);
    return 0;
}
