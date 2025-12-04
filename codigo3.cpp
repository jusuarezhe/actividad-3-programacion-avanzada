// domino_historial.cpp
// Basado en domino_fixed.cpp (usuario) -- añadido historial en lista enlazada
// Corregido: addHistory con 3 parámetros, filename unificado y pequeños fixes.

#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <random>
#include <ctime>
#include <limits>
#include <fstream>
#include <sstream>

using namespace std;

// ==================== Clase Ficha ====================
class Ficha {
private:
    int a, b;
public:
    Ficha(int aa = 0, int bb = 0) : a(aa), b(bb) {}
    int first() const { return a; }
    int second() const { return b; }
    void flip() { swap(a, b); }
    int pips() const { return a + b; }
    bool canConnect(int val) const { return a == val || b == val; }
    void print() const { cout << "[" << a << "|" << b << "]"; }
    string str() const {
        ostringstream ss; ss << "[" << a << "|" << b << "]"; 
        return ss.str();
    }
    bool equals(const Ficha& other) const {
        return (a == other.a && b == other.b) || (a == other.b && b == other.a);
    }
};

// ==================== Clase Jugador ====================
class Jugador {
private:
    string name;
    vector<Ficha> hand;
    int wins; // victorias acumuladas

public:
    Jugador(const string& n = "Jugador") : name(n), wins(0) {}

    const string& getName() const { return name; }
    int getWins() const { return wins; }
    void addWin() { ++wins; }

    void clearHand() { hand.clear(); }
    void receive(const Ficha& f) { hand.push_back(f); }
    int handSize() const { return (int)hand.size(); }
    int handPips() const {
        int s = 0;
        for (const auto& f : hand) s += f.pips();
        return s;
    }

    // mostramos la mano indexada desde 1
    void showHand() const {
        cout << "\nMano de " << name << " (" << hand.size() << "):\n";
        for (int i = 0; i < (int)hand.size(); ++i) {
            cout << i+1 << ". ";
            hand[i].print();
            cout << "  ";
            if ((i+1) % 6 == 0) cout << "\n";
        }
        cout << "\n";
    }

    bool hasPlayable(int left, int right) const {
        if (left == -1 && right == -1) return !hand.empty();
        for (const auto& f : hand) if (f.canConnect(left) || f.canConnect(right)) return true;
        return false;
    }

    // get copy of tile at index (0-based)
    Ficha tileAt(int idx) const { return hand[idx]; }

    // remove tile at idx and return it (ya devuelve copia)
    Ficha playAt(int idx) {
        Ficha t = hand[idx];
        hand.erase(hand.begin() + idx);
        return t;
    }
};

// ==================== Nodo de historial (lista enlazada) ====================
struct HistNode {
    string jugador;
    string ficha;    // texto de la ficha jugada, o "PASA"
    string tablero;  // estado completo del tablero después del movimiento
    HistNode* sig;
    HistNode(const string& j, const string& f, const string& t) : jugador(j), ficha(f), tablero(t), sig(nullptr) {}
};

// ==================== Clase JuegoDomino (con historial enlazado) ====================
class JuegoDomino {
private:
    vector<Ficha> deck;        // conjunto completo que se baraja
    deque<Ficha> table;        // mesa (deque para poder push_front/push_back)
    vector<Ficha> boneyard;    // fichas sobrantes (pozo) - por si se implementa robar
    vector<Jugador*> players;  // punteros para facilitar limpieza y polimorfismo futuro
    int currentIdx;            // índice del jugador actual
    mt19937 rng;

    // Historial enlazado (simplemente cabeza y cola para append eficiente)
    HistNode* histHead;
    HistNode* histTail;

    // crear 28 fichas
    void createDeck() {
        deck.clear();
        for (int i = 0; i <= 6; ++i)
            for (int j = i; j <= 6; ++j)
                deck.emplace_back(i, j);
    }

    // barajar deck usando rng
    void shuffleDeck() {
        shuffle(deck.begin(), deck.end(), rng);
    }

    // repartir 7 fichas por jugador; el resto a boneyard
    void dealHands() {
        for (auto p : players) p->clearHand();
        // deck ya barajado por quien llame a esta función
        int idx = 0;
        int per = 7;
        for (int r = 0; r < per; ++r) {
            for (auto p : players) {
                if (idx < (int)deck.size()) {
                    p->receive(deck[idx++]);
                }
            }
        }
        // restantes al boneyard (si quieres implementar robar)
        boneyard.clear();
        while (idx < (int)deck.size()) boneyard.push_back(deck[idx++]);
    }

    // determina quien comienza: mayor doble; si ninguno, ficha de mayor pips
    int determineStarter() {
        int starter = 0;
        int bestDouble = -1;
        for (int i = 0; i < (int)players.size(); ++i) {
            for (int k = 0; k < players[i]->handSize(); ++k) {
                const Ficha& f = players[i]->tileAt(k);
                if (f.first() == f.second()) {
                    if (f.first() > bestDouble) {
                        bestDouble = f.first();
                        starter = i;
                    }
                }
            }
        }
        if (bestDouble != -1) return starter;
        // otherwise highest pip tile
        int bestSum = -1;
        for (int i = 0; i < (int)players.size(); ++i) {
            for (int k = 0; k < players[i]->handSize(); ++k) {
                int s = players[i]->tileAt(k).pips();
                if (s > bestSum) { bestSum = s; starter = i; }
            }
        }
        return starter;
    }

    void showTable() const {
        cout << "\n--- MESA ---\n";
        if (table.empty()) cout << "(vacía)\n";
        else {
            for (const auto& f : table) { f.print(); cout << " "; }
            cout << "\nExtremos: " << table.front().first() << " ... " << table.back().second() << "\n";
        }
        cout << "-------------\n";
    }

    bool isBlocked() const {
        if (table.empty()) return false;
        int L = table.front().first();
        int R = table.back().second();
        for (auto p : players) if (p->hasPlayable(L, R)) return false;
        return true;
    }

    // Construye string que representa el tablero (lista de fichas)
    string tableToString() const {
        ostringstream ss;
        for (const auto& f : table) ss << f.str();
        return ss.str();
    }

    // Agrega una entrada al historial enlazado
    // ahora con estado opcional
    void addHistory(const string& jugador, const string& fichaText, const string& estado = "") {
        string estadoFinal = estado.empty() ? tableToString() : estado;
        HistNode* node = new HistNode(jugador, fichaText, estadoFinal);
        if (!histHead) {
            histHead = histTail = node;
        } else {
            histTail->sig = node;
            histTail = node;
        }
    }

    // Guarda historial en archivo "historial_domino.txt"
    void saveHistoryToFile() const {
        const string filename = "historial_domino.txt";
        ofstream ofs(filename);
        if (!ofs.is_open()) {
            cerr << "Error: no se pudo escribir el archivo de historial.\n";
            return;
        }
        ofs << "HISTORIAL DE MOVIMIENTOS - Dominó\n";
        ofs << "Formato: Jugador | Ficha (o PASA) | Estado del tablero después del movimiento\n\n";
        HistNode* cur = histHead;
        while (cur) {
            ofs << cur->jugador << " | " << cur->ficha << " | " << cur->tablero << "\n";
            cur = cur->sig;
        }
        ofs.close();
    }

    // Borra la lista de historial (libera memoria)
    void clearHistory() {
        HistNode* cur = histHead;
        while (cur) {
            HistNode* tmp = cur;
            cur = cur->sig;
            delete tmp;
        }
        histHead = histTail = nullptr;
    }

public:
    JuegoDomino() : currentIdx(0), histHead(nullptr), histTail(nullptr) {
        rng.seed(static_cast<unsigned>(time(nullptr)));
    }

    ~JuegoDomino() {
        // liberar memoria jugadores
        for (auto p : players) delete p;
        players.clear();
        // guardar historial (por si acaso) y liberar
        saveHistoryToFile();
        clearHistory();
    }

    // configurar N jugadores humanos
    void setupPlayersInteractive() {
        for (auto p : players) delete p;
        players.clear();

        int n;
        while (true) {
            cout << "¿Cuántos jugadores? (2-4): ";
            if (!(cin >> n)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); continue; }
            if (n >= 2 && n <= 4) break;
            cout << "Ingrese entre 2 y 4.\n";
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        for (int i = 0; i < n; ++i) {
            string nm;
            cout << "Nombre jugador " << i+1 << ": ";
            getline(cin, nm);
            if (nm.empty()) nm = "Jugador" + to_string(i+1);
            players.push_back(new Jugador(nm));
        }
    }

    // iniciar una ronda: crear deck, repartir y elegir quien inicia
    void startRound() {
        // limpiar historial anterior
        clearHistory();

        createDeck();
        shuffleDeck();
        dealHands();
        table.clear();
        currentIdx = determineStarter();
        cout << "\nInicia la ronda. Comienza: " << players[currentIdx]->getName() << "\n";
        playRound();
    }

    // lógica principal de la ronda (turnos secuenciales)
    void playRound() {
        int passesInRow = 0; // cuenta pases consecutivos para detectar bloqueo
        bool roundActive = true;

        while (roundActive) {
            showTable();

            Jugador* cur = players[currentIdx];
            cout << "\nTurno: " << cur->getName() << "\n";
            cur->showHand();

            int L = table.empty() ? -1 : table.front().first();
            int R = table.empty() ? -1 : table.back().second();

            bool playedThisTurn = false;

            // si no puede jugar y no hay mesa vacía -> pasar
            if (!table.empty() && !cur->hasPlayable(L,R)) {
                cout << cur->getName() << " no tiene jugadas válidas y pasa.\n";
                passesInRow++;
                // registramos PASS en historial (no se cambia el tablero)
                addHistory(cur->getName(), string("PASA"));
            } else {
                // pedir acción al jugador
                int choice = -1;
                while (true) {
                    cout << "Ingrese índice de ficha a jugar (1-" << cur->handSize() << ") o 0 para pasar: ";
                    if (!(cin >> choice)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); cout << "Entrada inválida.\n"; continue; }
                    if (choice == 0) { cout << cur->getName() << " pasa.\n"; passesInRow++; addHistory(cur->getName(), string("PASA")); break; }
                    int idx = choice - 1;
                    if (idx < 0 || idx >= cur->handSize()) { cout << "Índice fuera de rango.\n"; continue; }
                    Ficha candidate = cur->tileAt(idx);
                    // si mesa vacía aceptar directamente
                    if (table.empty()) {
                        Ficha played = cur->playAt(idx); // sacamos la ficha real
                        table.push_back(played);
                        cout << cur->getName() << " coloca "; table.back().print(); cout << " (mesa vacía)\n";
                        // registrar en historial (ficha jugada y estado del tablero)
                        addHistory(cur->getName(), played.str());
                        passesInRow = 0;
                        playedThisTurn = true;
                        break;
                    }
                    // else preguntar lado
                    char side;
                    cout << "Colocar en (I)zquierda o (D)erecha? ";
                    cin >> side; side = toupper(side);
                    if (side != 'I' && side != 'D') { cout << "Lado inválido.\n"; continue; }

                    int need = (side == 'I') ? L : R;
                    if (!candidate.canConnect(need)) { cout << "Esa ficha no encaja en ese lado.\n"; continue; }

                    // remove the real tile from player's hand, then orient that returned tile before inserting
                    Ficha played = cur->playAt(idx);
                    if (side == 'I') {
                        // queremos que played.second() == need
                        if (played.second() != need) played.flip();
                        table.push_front(played);
                        cout << cur->getName() << " coloca en izquierda "; table.front().print(); cout << "\n";
                    } else {
                        // queremos que played.first() == need
                        if (played.first() != need) played.flip();
                        table.push_back(played);
                        cout << cur->getName() << " coloca en derecha "; table.back().print(); cout << "\n";
                    }
                    // registrar en historial
                    addHistory(cur->getName(), played.str());
                    passesInRow = 0;
                    playedThisTurn = true;
                    break;
                } // end action loop
            }

            // check win
            if (cur->handSize() == 0) {
                cout << "\n***** " << cur->getName() << " se quedó sin fichas y gana la ronda! *****\n";
                cur->addWin();
                roundActive = false;
            } else {
                // check bloqueo: if passesInRow >= players.size() => blocked OR isBlocked()
                if (passesInRow >= (int)players.size() || isBlocked()) {
                    cout << "\n***** Ronda BLOQUEADA *****\n";
                    // determine winner by lowest hand pips
                    int winner = 0;
                    int minPips = players[0]->handPips();
                    for (int i = 1; i < (int)players.size(); ++i) {
                        int pips = players[i]->handPips();
                        cout << players[i]->getName() << " tiene " << pips << " pips.\n";
                        if (pips < minPips) { minPips = pips; winner = i; }
                    }
                    cout << players[winner]->getName() << " tiene menor pips (" << minPips << ") y gana la ronda.\n";
                    players[winner]->addWin();
                    // registrar en historial que la ronda terminó por bloqueo (registro final)
                    addHistory(string("SYSTEM"), string("BLOQUEO"), string("Ronda finalizada por bloqueo"));
                    roundActive = false;
                }
            }

            // Avanzar al siguiente jugador siempre si la ronda sigue
            if (roundActive) {
                currentIdx = (currentIdx + 1) % players.size();
            }
        } // end while roundActive

        // mostrar victorias acumuladas
        cout << "\nVictorias acumuladas:\n";
        for (auto p : players) cout << p->getName() << ": " << p->getWins() << "\n";

        // Al finalizar la ronda, guardamos historial
        saveHistoryToFile();
        cout << "\nHistorial guardado en 'historial_domino.txt'\n";
    }

    // menu principal / control de sesiones y reinicio
    void mainMenu() {
        bool running = true;
        while (running) {
            cout << "\n=== MENU PRINCIPAL ===\n";
            cout << "1) Configurar jugadores\n";
            cout << "2) Nueva ronda\n";
            cout << "3) Ver marcador\n";
            cout << "4) Reiniciar marcador y jugadores\n";
            cout << "5) Salir\n";
            cout << "Seleccione opcion: ";
            int opt;
            if (!(cin >> opt)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); continue; }
            switch (opt) {
                case 1:
                    setupPlayersInteractive();
                    break;
                case 2:
                    if (players.empty()) { cout << "Configure jugadores primero (opcion 1).\n"; break; }
                    startRound();
                    break;
                case 3:
                    for (auto p : players) cout << p->getName() << ": " << p->getWins() << " victorias\n";
                    break;
                case 4:
                    for (auto p : players) { p->clearHand(); delete p; }
                    players.clear();
                    cout << "Marcador y jugadores reiniciados. Configure nuevamente.\n";
                    break;
                case 5:
                    running = false;
                    break;
                default:
                    cout << "Opcion no valida.\n";
            }
        }
    }
};

// -----------------------------
// main
// -----------------------------
int main() {
    JuegoDomino game;
    game.mainMenu();
    cout << "Gracias por jugar. Fin.\n";
    return 0;
}
