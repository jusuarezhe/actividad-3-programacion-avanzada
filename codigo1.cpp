#include <iostream>
#include <fstream>
#include <string>
using namespace std;

// =====================================================
// ESTRUCTURA DEL ESTUDIANTE
// =====================================================
struct Estudiante {
    string nombreCompleto;
    int anoNacimiento;
    int identificacion;
    float promedio;
    string email;
};

// =====================================================
// NODO DE LA LISTA
// =====================================================
struct Nodo {
    Estudiante dato;
    Nodo* siguiente;

    Nodo(const Estudiante& est) : dato(est), siguiente(nullptr) {}
};

// =====================================================
// CLASE LISTADO ESTUDIANTIL (LISTA ENLAZADA SIMPLE)
// =====================================================
class ListadoEstudiantil {
private:
    Nodo* cabeza;
    int tamano;

public:

    // -------------------------------------------------
    // Constructor: lista vacía
    // -------------------------------------------------
    ListadoEstudiantil() : cabeza(nullptr), tamano(0) {}

    // -------------------------------------------------
    // Insertar al final
    // -------------------------------------------------
    void insertar(const Estudiante& est) {
        Nodo* nuevo = new Nodo(est);

        if (!cabeza) {
            cabeza = nuevo;
        } else {
            Nodo* temp = cabeza;
            while (temp->siguiente)
                temp = temp->siguiente;
            temp->siguiente = nuevo;
        }
        tamano++;
    }

    // -------------------------------------------------
    // Buscar por identificacion → devuelve posición
    // -------------------------------------------------
    int buscar(int id) {
        Nodo* temp = cabeza;
        int pos = 1;

        while (temp) {
            if (temp->dato.identificacion == id)
                return pos;
            temp = temp->siguiente;
            pos++;
        }
        return -1; // no encontrado
    }

    // -------------------------------------------------
    // Eliminar nodo por ID
    // -------------------------------------------------
    bool eliminar(int id) {
        if (!cabeza) return false;

        // Caso: eliminar cabeza
        if (cabeza->dato.identificacion == id) {
            Nodo* aux = cabeza;
            cabeza = cabeza->siguiente;
            delete aux;
            tamano--;
            return true;
        }

        // Buscar el nodo anterior
        Nodo* temp = cabeza;
        while (temp->siguiente &&
               temp->siguiente->dato.identificacion != id)
        {
            temp = temp->siguiente;
        }

        if (!temp->siguiente) return false;

        Nodo* eliminar = temp->siguiente;
        temp->siguiente = eliminar->siguiente;
        delete eliminar;
        tamano--;
        return true;
    }

    // -------------------------------------------------
    // Sobrecarga de operador [] para acceder por índice
    // -------------------------------------------------
    Estudiante operator[](int index) {
        if (index < 0 || index >= tamano) {
            throw out_of_range("Índice fuera de rango");
        }

        Nodo* temp = cabeza;
        for (int i = 0; i < index; i++)
            temp = temp->siguiente;

        return temp->dato;
    }

    // -------------------------------------------------
    // Imprimir en archivo TXT
    // -------------------------------------------------
    void imprimirTXT() {
        ofstream archivo("estudiantes.txt");

        if (!archivo.is_open()) {
            cout << "Error al crear archivo.\n";
            return;
        }

        Nodo* temp = cabeza;
        archivo << "===== LISTADO DE ESTUDIANTES =====\n\n";

        while (temp) {
            archivo << "Nombre: " << temp->dato.nombreCompleto << "\n";
            archivo << "Año nacimiento: " << temp->dato.anoNacimiento << "\n";
            archivo << "ID: " << temp->dato.identificacion << "\n";
            archivo << "Promedio: " << temp->dato.promedio << "\n";
            archivo << "Email: " << temp->dato.email << "\n";
            archivo << "----------------------------------\n";
            temp = temp->siguiente;
        }

        archivo.close();
        cout << "Archivo 'estudiantes.txt' generado correctamente.\n";
    }

    // -------------------------------------------------
    // Invertir la lista enlazada
    // -------------------------------------------------
    void invertir() {
        Nodo* prev = nullptr;
        Nodo* actual = cabeza;
        Nodo* siguiente = nullptr;

        while (actual) {
            siguiente = actual->siguiente;
            actual->siguiente = prev;
            prev = actual;
            actual = siguiente;
        }

        cabeza = prev;
    }

    // -------------------------------------------------
    // Destructor: liberar memoria
    // -------------------------------------------------
    ~ListadoEstudiantil() {
        Nodo* temp;
        while (cabeza) {
            temp = cabeza;
            cabeza = cabeza->siguiente;
            delete temp;
        }
    }

    int size() { return tamano; }
};

// =====================================================
// MENÚ INTERACTIVO PARA OPERAR LA LISTA
// =====================================================
void mostrarMenu() {
    cout << "\n===== MENU LISTADO ESTUDIANTIL =====\n";
    cout << "1. Insertar estudiante\n";
    cout << "2. Buscar estudiante\n";
    cout << "3. Eliminar estudiante\n";
    cout << "4. Imprimir lista en archivo TXT\n";
    cout << "5. Invertir lista\n";
    cout << "6. Mostrar estudiante por indice\n";
    cout << "7. Salir\n";
    cout << "Seleccione opcion: ";
}

// =====================================================
// PROGRAMA PRINCIPAL
// =====================================================
int main() {
    ListadoEstudiantil lista;
    int opcion;

    do {
        mostrarMenu();
        cin >> opcion;

        if (opcion == 1) {
            Estudiante est;
            cout << "Nombre completo: ";
            cin.ignore();
            getline(cin, est.nombreCompleto);

            cout << "Año nacimiento: ";
            cin >> est.anoNacimiento;

            cout << "Identificación: ";
            cin >> est.identificacion;

            cout << "Promedio academico: ";
            cin >> est.promedio;

            cout << "Email: ";
            cin >> est.email;

            lista.insertar(est);
            cout << "Estudiante agregado correctamente.\n";
        }

        else if (opcion == 2) {
            int id;
            cout << "Ingrese ID a buscar: ";
            cin >> id;

            int pos = lista.buscar(id);
            if (pos == -1)
                cout << "No encontrado.\n";
            else
                cout << "Encontrado en la posición: " << pos << endl;
        }

        else if (opcion == 3) {
            int id;
            cout << "Ingrese ID a eliminar: ";
            cin >> id;

            if (lista.eliminar(id))
                cout << "Eliminado correctamente.\n";
            else
                cout << "No se encontró ese ID.\n";
        }

        else if (opcion == 4) {
            lista.imprimirTXT();
        }

        else if (opcion == 5) {
            lista.invertir();
            cout << "Lista invertida correctamente.\n";
        }

        else if (opcion == 6) {
            int index;
            cout << "Ingrese índice (0 a " << lista.size()-1 << "): ";
            cin >> index;

            try {
                Estudiante e = lista[index];
                cout << "\n=== ESTUDIANTE EN INDICE " << index << " ===\n";
                cout << "Nombre: " << e.nombreCompleto << endl;
                cout << "Año nacimiento: " << e.anoNacimiento << endl;
                cout << "ID: " << e.identificacion << endl;
                cout << "Promedio: " << e.promedio << endl;
                cout << "Email: " << e.email << endl;
            } catch (exception &ex) {
                cout << "Error: " << ex.what() << endl;
            }
        }

    } while (opcion != 7);

    return 0;
}
