// SenalECG.cpp
// Clase SenalECG con lista doblemente enlazada, filtros (IIR y FIR moving-average),
// detección de picos y cálculo de frecuencia cardíaca promedio.
// Versión estilo "estudiante intermedio": comentarios en primera persona y
// algunos detalles intencionales de estilo (no afectan funcionamiento).

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include <algorithm>

using namespace std;

// -------------------------------------------------
// Nodo doble para almacenar cada muestra
// -------------------------------------------------
struct Nodo {
    double tiempo;     // instante en segundos
    double amplitud;   // señal original
    double filtrada;   // señal filtrada (resultado)
    Nodo* sig;
    Nodo* ant;
    Nodo(double t = 0.0, double a = 0.0)
        : tiempo(t), amplitud(a), filtrada(0.0), sig(nullptr), ant(nullptr) {}
};

// -------------------------------------------------
// Clase SenalECG
// -------------------------------------------------
class SenalECG {
private:
    Nodo* inicio;
    Nodo* fin;
    size_t tam;   // cantidad de muestras

public:
    // Constructor: inicializamos lista vacía
    SenalECG() : inicio(nullptr), fin(nullptr), tam(0) {
        // nosotros iniciamos vacío, tal cual nos pidieron
    }

    // Destructor: liberamos memoria dinámica
    ~SenalECG() {
        Nodo* cur = inicio;
        while (cur) {
            Nodo* tmp = cur;
            cur = cur->sig;
            delete tmp;
        }
        // memoria liberada
    }

    // Insertar al final (append)
    void insertar(double t, double a) {
        Nodo* n = new Nodo(t, a);
        if (!inicio) {
            inicio = fin = n;
        } else {
            fin->sig = n;
            n->ant = fin;
            fin = n;
        }
        ++tam;
    }

    // Cargar archivo de texto. Cada línea: tiempo amplitud  (o amplitud tiempo, pero esperamos tiempo primero)
    bool cargarDesdeArchivo(const string& nombreArchivo) {
        ifstream ifs(nombreArchivo);
        if (!ifs.is_open()) {
            cerr << "Error: no se pudo abrir '" << nombreArchivo << "'\n";
            return false;
        }
        double t, a;
        size_t cont = 0;
        // Detectamos si el archivo entrega "amplitud tiempo" o "tiempo amplitud"?
        // Asumimos tiempo primero (t a). Si tus datos están invertidos, cambia orden afuera.
        while (ifs >> t >> a) {
            insertar(t, a);
            ++cont;
        }
        ifs.close();
        if (cont == 0) {
            cerr << "Advertencia: archivo leído pero sin datos.\n";
            return false;
        }
        cout << "Leídas " << cont << " muestras desde '" << nombreArchivo << "'.\n";
        return true;
    }

    // Devuelve tamaño
    size_t size() const { return tam; }

    // -------------------------------------------------
    // FILTRO IIR simple (pasa-bajos exponencial)
    // y[n] = alpha * y[n-1] + (1-alpha) * x[n]
    // alpha en (0,1). alpha cercano a 1 => más suavizado.
    // -------------------------------------------------
    void filtroIIR(double alpha = 0.98) {
        if (!inicio) return;
        if (alpha < 0.0) alpha = 0.0;
        if (alpha > 1.0) alpha = 1.0;

        // Inicialización: primer valor filtrada = primer valor original
        inicio->filtrada = inicio->amplitud;
        double y_prev = inicio->filtrada;

        Nodo* cur = inicio->sig;
        while (cur) {
            cur->filtrada = alpha * y_prev + (1.0 - alpha) * cur->amplitud;
            y_prev = cur->filtrada;
            cur = cur->sig;
        }

        // Nota: usamos inicio->filtrada = inicio->amplitud para evitar salto inicial
        cout << "[Filtro IIR] aplicado con alpha=" << alpha << "\n";
    }

    // -------------------------------------------------
    // FILTRO FIR: Moving average (ventana centrada)
    // ventanaLong = cantidad total de muestras en la ventana (debe ser >=1).
    // Si ventanaLong es par, el código lo adapta usando ventana centrada de longitud ventanaLong.
    // -------------------------------------------------
    void filtroFIR_movingAverage(int ventanaLong = 51) {
        if (!inicio) return;
        if (ventanaLong < 1) ventanaLong = 1;

        // Cargar señal original en vector para procesamiento cómodo
        vector<double> buffer;
        buffer.reserve(tam);
        for (Nodo* p = inicio; p; p = p->sig) buffer.push_back(p->amplitud);

        int N = ventanaLong;
        // Para ventana centrada usaremos radio = N/2 (entero), ventana real = 2*radio+1 si queremos impar
        int radio = N / 2;
        int L = 2 * radio + 1; // longitud impar (si N era par, ajusta a impar más cercana)
        if (L <= 0) L = 1;

        vector<double> salida(buffer.size(), 0.0);

        // Para cada muestra i, calculamos promedio de las muestras en [i-radio, i+radio] que existan
        for (int i = 0; i < (int)buffer.size(); ++i) {
            double suma = 0.0;
            int cuenta = 0;
            int desde = max(0, i - radio);
            int hasta = min((int)buffer.size() - 1, i + radio);
            for (int k = desde; k <= hasta; ++k) {
                suma += buffer[k];
                ++cuenta;
            }
            if (cuenta > 0) salida[i] = suma / cuenta;
            else salida[i] = buffer[i];
        }

        // Guardar resultado en lista doble enlazada (campo filtrada)
        int idx = 0;
        for (Nodo* p = inicio; p; p = p->sig, ++idx) {
            p->filtrada = salida[idx];
        }

        cout << "[Filtro FIR] moving-average aplicado. Ventana aprox=" << L << " muestras (radio=" << radio << ").\n";
    }

    // -------------------------------------------------
    // Detección de picos (R-peaks) sobre la señal filtrada.
    // Estrategia:
    //  - Detectar máximos locales: y[n] > y[n-1] && y[n] > y[n+1]
    //  - Aplicar umbral (adaptativo): umbral = max(umbralMin, K*rms)
    //  - Aplicar refractorio mínimo en segundos para no detectar el mismo QRS varias veces.
    // Devuelve vector de tiempos de picos.
    // -------------------------------------------------
    vector<double> detectarPicos(double umbralMin = 0.5, double refractario_seg = 0.25) {
        vector<double> tiempos;
        if (tam < 3) return tiempos;

        // 1) calcular RMS y max absoluto de la señal filtrada
        double sum2 = 0.0;
        double maxAbs = 0.0;
        size_t n = 0;
        for (Nodo* p = inicio; p; p = p->sig) {
            double v = fabs(p->filtrada);
            sum2 += v * v;
            if (v > maxAbs) maxAbs = v;
            ++n;
        }
        double rms = (n > 0) ? sqrt(sum2 / n) : 0.0;

        // 2) umbral adaptativo
        double umbral = max(umbralMin, rms * 1.2);
        // si maxAbs es pequeño, permitimos umbral más pequeño
        if (maxAbs > 0 && umbral > 0.9 * maxAbs) umbral = 0.6 * maxAbs;

        // 3) recorre y detecta maximos locales
        Nodo* cur = inicio->sig; // empezamos en segundo elemento
        double ultimo_pico_t = -1e9;

        while (cur && cur->sig) {
            double y = cur->filtrada;
            if (y > cur->ant->filtrada && y > cur->sig->filtrada && fabs(y) >= umbral) {
                double tcur = cur->tiempo;
                if (tcur - ultimo_pico_t >= refractario_seg) {
                    tiempos.push_back(tcur);
                    ultimo_pico_t = tcur;
                } else {
                    // dentro del refractario: se ignora
                }
            }
            cur = cur->sig;
        }

        cout << "Detectados " << tiempos.size() << " picos (umbral=" << umbral << ", rms=" << rms << ").\n";
        return tiempos;
    }

    // -------------------------------------------------
    // Guardar señal (tiempo, original, filtrada) en archivo
    // -------------------------------------------------
    bool guardarEnArchivo(const string& nombreSalida) const {
        ofstream ofs(nombreSalida);
        if (!ofs.is_open()) {
            cerr << "Error: no se pudo crear '" << nombreSalida << "'\n";
            return false;
        }
        ofs << "tiempo\toriginal\tfiltrada\n";
        for (Nodo* p = inicio; p; p = p->sig) {
            ofs << p->tiempo << "\t" << p->amplitud << "\t" << p->filtrada << "\n";
        }
        ofs.close();
        cout << "Archivo '" << nombreSalida << "' escrito OK.\n";
        return true;
    }

    // -------------------------------------------------
    // Calcular frecuencia cardiaca promedio (bpm) a partir de picos detectados
    // -------------------------------------------------
    double frecuenciaCardiacaPromedio(double umbralMin = 0.5, double refractario_seg = 0.25) {
        vector<double> picos = detectarPicos(umbralMin, refractario_seg);
        if (picos.size() < 2) {
            cerr << "No hay suficientes picos para calcular frecuencia.\n";
            return 0.0;
        }
        double sumaRR = 0.0;
        for (size_t i = 1; i < picos.size(); ++i) sumaRR += (picos[i] - picos[i - 1]);
        double rr_prom = sumaRR / (picos.size() - 1);
        if (rr_prom <= 0.0) return 0.0;
        double bpm = 60.0 / rr_prom;
        return bpm;
    }

    // -------------------------------------------------
    // Debug: imprimir primeras N muestras (tiempo original filtrada)
    // -------------------------------------------------
    void debugPrint(int n = 10) const {
        cout << "Primeras " << n << " muestras (tiempo original filtrada):\n";
        int i = 0;
        for (Nodo* p = inicio; p && i < n; p = p->sig, ++i) {
            cout << p->tiempo << "\t" << p->amplitud << "\t" << p->filtrada << "\n";
        }
    }
};

// -------------------------------------------------
// FUNCIONES AUXILIARES PARA INTERFAZ
// -------------------------------------------------
void limpiarEntrada() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void menuPrincipal() {
    cout << "\n=============== MENU SenalECG ===============\n";
    cout << "1) Cargar archivo ECG (texto)\n";
    cout << "2) Aplicar filtro IIR (alpha)\n";
    cout << "3) Aplicar filtro FIR (moving-average)\n";
    cout << "4) Detectar picos y mostrar tiempos\n";
    cout << "5) Calcular frecuencia cardiaca promedio (bpm)\n";
    cout << "6) Guardar señ1al original+filtrada en archivo\n";
    cout << "7) Mostrar primeras muestras (debug)\n";
    cout << "8) Salir\n";
    cout << "Seleccione opcion: ";
}

// -------------------------------------------------
// MAIN
// -------------------------------------------------
int main() {
    SenalECG s;
    bool cargado = false;
    int opcion = 0;

    while (true) {
        menuPrincipal();
        if (!(cin >> opcion)) {
            cout << "Entrada invalida. Intenta otra vez.\n";
            limpiarEntrada();
            continue;
        }

        if (opcion == 1) {
            string nombre;
            cout << "Nombre del archivo a cargar (ej: ECG.txt): ";
            limpiarEntrada();
            getline(cin, nombre);
            if (nombre.empty()) nombre = "ECG.txt";
            if (s.cargarDesdeArchivo(nombre)) {
                cargado = true;
            } else {
                cargado = false;
            }
        }
        else if (opcion == 2) {
            if (!cargado) { cout << "Primero cargue un archivo (opcion 1).\n"; continue; }
            double alpha;
            cout << "Alpha (0..1) para IIR (ej: 0.98): ";
            if (!(cin >> alpha)) { cout << "Valor invalido.\n"; limpiarEntrada(); continue; }
            s.filtroIIR(alpha);
        }
        else if (opcion == 3) {
            if (!cargado) { cout << "Primero cargue un archivo (opcion 1).\n"; continue; }
            int ventana;
            cout << "Longitud ventana (entera, p.ej. 51 - mayor = mas suave): ";
            if (!(cin >> ventana)) { cout << "Valor invalido.\n"; limpiarEntrada(); continue; }
            s.filtroFIR_movingAverage(ventana);
        }
        else if (opcion == 4) {
            if (!cargado) { cout << "Primero cargue un archivo (opcion 1).\n"; continue; }
            double umbral; double refract;
            cout << "Umbral minimo (ej: 0.5): ";
            if (!(cin >> umbral)) { cout << "Valor invalido.\n"; limpiarEntrada(); continue; }
            cout << "Periodo refractario (s) (ej: 0.25): ";
            if (!(cin >> refract)) { cout << "Valor invalido.\n"; limpiarEntrada(); continue; }
            vector<double> picos = s.detectarPicos(umbral, refract);
            if (picos.empty()) cout << "No se detectaron picos.\n";
            else {
                cout << "Tiempos de picos detectados (s):\n";
                for (double t : picos) cout << t << "\n";
            }
        }
        else if (opcion == 5) {
            if (!cargado) { cout << "Primero cargue un archivo (opcion 1).\n"; continue; }
            double umbral; double refract;
            cout << "Umbral minimo (ej: 0.5): ";
            if (!(cin >> umbral)) { cout << "Valor invalido.\n"; limpiarEntrada(); continue; }
            cout << "Periodo refractario (s) (ej: 0.25): ";
            if (!(cin >> refract)) { cout << "Valor invalido.\n"; limpiarEntrada(); continue; }
            double bpm = s.frecuenciaCardiacaPromedio(umbral, refract);
            if (bpm > 0.0) cout << "Frecuencia cardiaca promedio estimada: " << bpm << " bpm\n";
            else cout << "No se pudo estimar la frecuencia (pocos picos detectados).\n";
        }
        else if (opcion == 6) {
            if (!cargado) { cout << "Primero cargue un archivo (opcion 1).\n"; continue; }
            string nombreOut;
            limpiarEntrada();
            cout << "Nombre de archivo de salida (ej: ECG_filtrada.txt): ";
            getline(cin, nombreOut);
            if (nombreOut.empty()) nombreOut = "ECG_filtrada.txt";
            s.guardarEnArchivo(nombreOut);
        }
        else if (opcion == 7) {
            int n;
            cout << "Cuantas muestras mostrar (ej: 12): ";
            if (!(cin >> n)) { cout << "Valor invalido.\n"; limpiarEntrada(); continue; }
            s.debugPrint(n);
        }
        else if (opcion == 8) {
            cout << "Saliendo...\n";
            break;
        }
        else {
            cout << "Opcion no valida.\n";
        }
        // dejamos una línea en blanco para separar iteraciones (estética)
        cout << "\n";
    }

    return 0;
}
