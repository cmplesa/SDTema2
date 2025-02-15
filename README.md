#311CA_PlesaMarian-Cosmin_Tema2-2024

# Descriere

Am inceput tema prin a lucra la fisierele .c in urmatoarea ordine:
1. lru_cache.c
2. server.c
3. load_balancer.c
Am realizat implementarea pentru un server, dupa am testat si am continuat
sa realizez implementarea pentru mai multe servere.

# LRU_CACHE.C

Am implementat LRU care se bazeaza pe stergerea ultimei intrari care a fost
accesate. Astfel mereu am adaugat in lista de ordine la inceput si mereu
evicted key a fost ultimul nod din coada. Precum in cerinta am implementat
LRU prin structura ceruta:
1. lista dublu inlantuita care retine ordinea
2. hashmap

Am realizat functia de put care verifica existenta unui document in cache si in
acest caz am dat return true, iar dupa am verificat daca cache-ul este full si
am atribuit ori NULL ori efectiv nume de document variabilei evicted_key
care m-a ajutat sa retin ce a fost scos din cache pentru a face loc altei intrari.

Am realizat si functia de get apeland functii implementate la laborator, am returnat
direct prin apelarea functiei ht_get.

# SERVER.C

Am implementat serverul prin structura urmatoare:
1. lru_cache
2. o lista dublu inlantuita pentru DB
3. o coada care retine taskurile de tip EDIT

Am initializat serverul si am facut functia de free prima oara si dupa m-am ocupat
de functia server_handle_request.

Pentru un request de tip EDIT DOCUMENT am facut exact ca in cerinta,
am adaugat requestul in coada si dupa am intors o structura de tip RESPONSE 
care contine MSG_A si LOG_LAZY_EXEC.

Pentru requesturile de tip GET, am realizat o functie dotask care sa faca toate
taskurile de tip EDIT din coada si am urmarit arborele din cerinta pe partea de edit.
Dupa realizarea acestor taskuri se face requestul de tip get si am urmarit partea
de get al aroberelui pus la dispozitie si am pus mesajele corespunzatoare.
