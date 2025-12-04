#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include <algorithm>

using namespace std;

// Nodo de la lista doble: guarda tiempo, amplitud y valor filtrado
struct Nodo {
    double tiempo;
    double amplitud;
    double filtrada;
    Nodo* sig;
    Nodo* ant;

    Nodo(double t = 0.0, double a = 0.0)
        : tiempo(t), amplitud(a), filtrada(0.0), sig(nullptr), ant(nullptr) {}
};

// Clase que maneja la señal ECG usando lista doble enlazada
class SenalECG {
private:
    Nodo* inicio;
    Nodo* fin;
    size_t tam;

public:
    // Lista vacía
    SenalECG() : inicio(nullptr), fin(nullptr), tam(0) {}

    // Liberamos memoria al terminar
    ~SenalECG() {
        Nodo* cur = inicio;
        while (cur) {
            Nodo* tmp = cur;
            cur = cur->sig;
            delete tmp;
        }
    }

    // Insertar muestra al final
    void insertar(double t, double a) {
        Nodo* n = new Nodo(t, a);
        if (!inicio) {
            inicio = fin = n;
        } else {
            fin->sig = n;
            n->ant = fin;
            fin = n;
        }
        tam++;
    }

    // Cargar datos desde archivo
    bool cargarDesdeArchivo(const string& nombreArchivo) {
        ifstream ifs(nombreArchivo);
        if (!ifs.is_open()) {
            cerr << "Error abriendo archivo.\n";
            return false;
        }

        double t, a;
        size_t cont = 0;

        while (ifs >> t >> a) {
            insertar(t, a);
            cont++;
        }
        ifs.close();

        if (cont == 0) {
            cerr << "Archivo vacío.\n";
            return false;
        }

        cout << "Leídas " << cont << " muestras.\n";
        return true;
    }

    size_t size() const { return tam; }

    // Filtro IIR simple
    void filtroIIR(double alpha = 0.98) {
        if (!inicio) return;
        if (alpha < 0) alpha = 0;
        if (alpha > 1) alpha = 1;

        inicio->filtrada = inicio->amplitud;
        double y_prev = inicio->filtrada;

        Nodo* cur = inicio->sig;
        while (cur) {
            cur->filtrada = alpha * y_prev + (1 - alpha) * cur->amplitud;
            y_prev = cur->filtrada;
            cur = cur->sig;
        }
        cout << "Filtro IIR aplicado.\n";
    }

    // Filtro FIR (promedio móvil)
    void filtroFIR_movingAverage(int ventanaLong = 51) {
        if (!inicio) return;

        vector<double> buffer;
        buffer.reserve(tam);
        for (Nodo* p = inicio; p; p = p->sig) buffer.push_back(p->amplitud);

        int N = ventanaLong;
        int radio = N / 2;
        int L = 2 * radio + 1;
        if (L <= 0) L = 1;

        vector<double> salida(buffer.size(), 0.0);

        for (int i = 0; i < (int)buffer.size(); i++) {
            double suma = 0;
            int cuenta = 0;
            int ini = max(0, i - radio);
            int fin = min((int)buffer.size() - 1, i + radio);

            for (int k = ini; k <= fin; k++) {
                suma += buffer[k];
                cuenta++;
            }

            salida[i] = (cuenta > 0 ? suma / cuenta : buffer[i]);
        }

        int idx = 0;
        for (Nodo* p = inicio; p; p = p->sig, idx++) {
            p->filtrada = salida[idx];
        }

        cout << "Filtro FIR aplicado.\n";
    }

    // Detección de picos básicos
    vector<double> detectarPicos(double umbralMin = 0.5, double refractario_seg = 0.25) {
        vector<double> tiempos;
        if (tam < 3) return tiempos;

        double sum2 = 0, maxAbs = 0;
        size_t n = 0;

        for (Nodo* p = inicio; p; p = p->sig) {
            double v = fabs(p->filtrada);
            sum2 += v * v;
            maxAbs = max(maxAbs, v);
            n++;
        }

        double rms = (n > 0 ? sqrt(sum2 / n) : 0);
        double umbral = max(umbralMin, rms * 1.2);
        if (maxAbs > 0 && umbral > 0.9 * maxAbs) umbral = 0.6 * maxAbs;

        Nodo* cur = inicio->sig;
        double ultimo = -1e9;

        while (cur && cur->sig) {
            double y = cur->filtrada;

            if (y > cur->ant->filtrada && y > cur->sig->filtrada && fabs(y) >= umbral) {
                if (cur->tiempo - ultimo >= refractario_seg) {
                    tiempos.push_back(cur->tiempo);
                    ultimo = cur->tiempo;
                }
            }
            cur = cur->sig;
        }

        cout << "Picos detectados: " << tiempos.size() << "\n";
        return tiempos;
    }

    // Guardar señal procesada
    bool guardarEnArchivo(const string& nombreSalida) const {
        ofstream ofs(nombreSalida);
        if (!ofs.is_open()) {
            cerr << "Error guardando archivo.\n";
            return false;
        }

        ofs << "tiempo\toriginal\tfiltrada\n";
        for (Nodo* p = inicio; p; p = p->sig)
            ofs << p->tiempo << "\t" << p->amplitud << "\t" << p->filtrada << "\n";

        ofs.close();
        cout << "Archivo guardado.\n";
        return true;
    }

    // Calcular frecuencia cardíaca
    double frecuenciaCardiacaPromedio(double umbralMin = 0.5, double refractario_seg = 0.25) {
        vector<double> picos = detectarPicos(umbralMin, refractario_seg);
        if (picos.size() < 2) return 0;

        double sumaRR = 0;
        for (size_t i = 1; i < picos.size(); i++)
            sumaRR += (picos[i] - picos[i - 1]);

        double rr_prom = sumaRR / (picos.size() - 1);
        return (rr_prom > 0 ? 60.0 / rr_prom : 0);
    }

    // Mostrar algunas muestras
    void debugPrint(int n = 10) const {
        cout << "Primeras " << n << " muestras:\n";
        int i = 0;
        for (Nodo* p = inicio; p && i < n; p = p->sig, i++)
            cout << p->tiempo << "\t" << p->amplitud << "\t" << p->filtrada << "\n";
    }
};

// Limpia entrada del usuario
void limpiarEntrada() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Menú simple
void menuPrincipal() {
    cout << "\n=== MENU ECG ===\n";
    cout << "1) Cargar archivo\n";
    cout << "2) Filtro IIR\n";
    cout << "3) Filtro FIR\n";
    cout << "4) Detectar picos\n";
    cout << "5) Frecuencia cardiaca\n";
    cout << "6) Guardar señal\n";
    cout << "7) Mostrar muestras\n";
    cout << "8) Salir\n";
    cout << "Opción: ";
}

int main() {
    SenalECG s;
    bool cargado = false;
    int opcion = 0;

    while (true) {
        menuPrincipal();

        if (!(cin >> opcion)) {
            limpiarEntrada();
            continue;
        }

        if (opcion == 1) {
            string nombre;
            cout << "Archivo: ";
            limpiarEntrada();
            getline(cin, nombre);
            if (nombre.empty()) nombre = "ECG.txt";
            cargado = s.cargarDesdeArchivo(nombre);
        }

        else if (opcion == 2) {
            if (!cargado) { cout << "Cargue un archivo primero.\n"; continue; }
            double alpha;
            cout << "Alpha: ";
            if (!(cin >> alpha)) { limpiarEntrada(); continue; }
            s.filtroIIR(alpha);
        }

        else if (opcion == 3) {
            if (!cargado) { cout << "Cargue un archivo primero.\n"; continue; }
            int ventana;
            cout << "Tamaño ventana: ";
            if (!(cin >> ventana)) { limpiarEntrada(); continue; }
            s.filtroFIR_movingAverage(ventana);
        }

        else if (opcion == 4) {
            if (!cargado) { cout << "Cargue primero.\n"; continue; }
            double u, r;
            cout << "Umbral: ";
            if (!(cin >> u)) { limpiarEntrada(); continue; }
            cout << "Refractario: ";
            if (!(cin >> r)) { limpiarEntrada(); continue; }
            vector<double> p = s.detectarPicos(u, r);
            for (double t : p) cout << t << "\n";
        }

        else if (opcion == 5) {
            if (!cargado) { cout << "Cargue primero.\n"; continue; }
            double u, r;
            cout << "Umbral: ";
            if (!(cin >> u)) { limpiarEntrada(); continue; }
            cout << "Refractario: ";
            if (!(cin >> r)) { limpiarEntrada(); continue; }
            double bpm = s.frecuenciaCardiacaPromedio(u, r);
            cout << "BPM: " << bpm << "\n";
        }

        else if (opcion == 6) {
            if (!cargado) { cout << "Cargue primero.\n"; continue; }
            string out;
            limpiarEntrada();
            cout << "Archivo salida: ";
            getline(cin, out);
            if (out.empty()) out = "ECG_filtrada.txt";
            s.guardarEnArchivo(out);
        }

        else if (opcion == 7) {
            int n;
            cout << "Cantidad: ";
            if (!(cin >> n)) { limpiarEntrada(); continue; }
            s.debugPrint(n);
        }

        else if (opcion == 8) {
            cout << "Saliendo...\n";
            break;
        }
    }

    return 0;
}
