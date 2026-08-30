/*
 * Métodos Numéricos - Derivación e Integración
 * Temas: 3.1 Derivación Numérica
 *        3.2 Newton-Cotes (Trapecio, Simpson 1/3, Simpson 3/8)
 *        3.3 Integración de Romberg
 *
 * Uso: El usuario ingresa f(x) como expresión matemática.
 * Requiere: compilar con -lm
 */

#include <iostream>
#include <cmath>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <functional>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  EVALUADOR DE EXPRESIONES MATEMÁTICAS
//  Soporta: +, -, *, /, ^, (, ), sin, cos, tan, exp, log, ln, sqrt, abs, pi, e
// ─────────────────────────────────────────────────────────────────────────────
struct Token {
    enum Type { NUMBER, VARIABLE, FUNC, OP, LPAREN, RPAREN, END } type;
    double number;
    string str;
};

class ExprEval {
    string expr;
    size_t pos;
    double xval;

    Token nextToken() {
        while (pos < expr.size() && isspace(expr[pos])) pos++;
        if (pos >= expr.size()) return {Token::END, 0, ""};

        char c = expr[pos];

        if (isdigit(c) || c == '.') {
            size_t start = pos;
            while (pos < expr.size() && (isdigit(expr[pos]) || expr[pos] == '.')) pos++;
            if (pos < expr.size() && expr[pos] == 'e') {
                pos++;
                if (pos < expr.size() && (expr[pos]=='+' || expr[pos]=='-')) pos++;
                while (pos < expr.size() && isdigit(expr[pos])) pos++;
            }
            return {Token::NUMBER, stod(expr.substr(start, pos-start)), ""};
        }

        if (isalpha(c) || c == '_') {
            size_t start = pos;
            while (pos < expr.size() && (isalnum(expr[pos]) || expr[pos]=='_')) pos++;
            string name = expr.substr(start, pos-start);
            if (name == "x") return {Token::VARIABLE, 0, "x"};
            if (name == "pi") return {Token::NUMBER, M_PI, ""};
            if (name == "e" && (pos >= expr.size() || !isalnum(expr[pos])))
                return {Token::NUMBER, M_E, ""};
            return {Token::FUNC, 0, name};
        }

        if (c == '(') { pos++; return {Token::LPAREN, 0, "("}; }
        if (c == ')') { pos++; return {Token::RPAREN, 0, ")"}; }
        if (string("+-*/^").find(c) != string::npos) { pos++; return {Token::OP, 0, string(1,c)}; }

        throw runtime_error(string("Carácter no reconocido: ") + c);
    }

    // Gramática: expr = term (('+' | '-') term)*
    // term = factor (('*' | '/') factor)*
    // factor = base ('^' factor)?
    // base = number | variable | func '(' expr ')' | '-' base | '(' expr ')'

    Token current;
    bool have_current = false;

    Token peek() {
        if (!have_current) { current = nextToken(); have_current = true; }
        return current;
    }
    Token consume() {
        Token t = peek(); have_current = false; return t;
    }

    double parseExpr();
    double parseTerm();
    double parseFactor();
    double parseBase();

    double applyFunc(const string& name, double arg) {
        if (name == "sin")  return sin(arg);
        if (name == "cos")  return cos(arg);
        if (name == "tan")  return tan(arg);
        if (name == "exp")  return exp(arg);
        if (name == "log")  return log10(arg);
        if (name == "ln")   return log(arg);
        if (name == "sqrt") return sqrt(arg);
        if (name == "abs")  return fabs(arg);
        if (name == "asin") return asin(arg);
        if (name == "acos") return acos(arg);
        if (name == "atan") return atan(arg);
        if (name == "cosh") return cosh(arg);
        if (name == "sinh") return sinh(arg);
        if (name == "tanh") return tanh(arg);
        throw runtime_error("Función desconocida: " + name);
    }

public:
    double evaluate(const string& expression, double x) {
        expr = expression; pos = 0; xval = x; have_current = false;
        double result = parseExpr();
        return result;
    }
};

double ExprEval::parseExpr() {
    double val = parseTerm();
    while (peek().type == Token::OP && (peek().str == "+" || peek().str == "-")) {
        Token op = consume();
        double rhs = parseTerm();
        if (op.str == "+") val += rhs; else val -= rhs;
    }
    return val;
}
double ExprEval::parseTerm() {
    double val = parseFactor();
    while (peek().type == Token::OP && (peek().str == "*" || peek().str == "/")) {
        Token op = consume();
        double rhs = parseFactor();
        if (op.str == "*") val *= rhs; else { if (rhs==0) throw runtime_error("División por cero"); val /= rhs; }
    }
    return val;
}
double ExprEval::parseFactor() {
    double base = parseBase();
    if (peek().type == Token::OP && peek().str == "^") {
        consume();
        double exp_val = parseFactor(); // right-associative
        return pow(base, exp_val);
    }
    return base;
}
double ExprEval::parseBase() {
    Token t = peek();
    if (t.type == Token::OP && t.str == "-") { consume(); return -parseBase(); }
    if (t.type == Token::OP && t.str == "+") { consume(); return parseBase(); }
    if (t.type == Token::NUMBER)  { consume(); return t.number; }
    if (t.type == Token::VARIABLE){ consume(); return xval; }
    if (t.type == Token::LPAREN)  {
        consume();
        double val = parseExpr();
        if (consume().type != Token::RPAREN) throw runtime_error("Falta )");
        return val;
    }
    if (t.type == Token::FUNC) {
        consume();
        if (peek().type != Token::LPAREN) throw runtime_error("Se esperaba ( después de " + t.str);
        consume();
        double arg = parseExpr();
        if (consume().type != Token::RPAREN) throw runtime_error("Falta ) en función");
        return applyFunc(t.str, arg);
    }
    throw runtime_error("Token inesperado: " + t.str);
}

// ─────────────────────────────────────────────────────────────────────────────
//  UTILIDADES
// ─────────────────────────────────────────────────────────────────────────────
ExprEval evaluator;
string currentFunc;

double f(double x) {
    return evaluator.evaluate(currentFunc, x);
}

void leerFuncion() {
    cout << "\n  Ingrese f(x) (use x como variable):\n";
    cout << "  Funciones disponibles: sin, cos, tan, exp, ln, log, sqrt, abs, pi, e\n";
    cout << "  Potencias con ^,  Ej: x^3 - 2*x + sin(x)\n";
    cout << "  f(x) = ";
    cin.ignore();
    getline(cin, currentFunc);
    // prueba rápida
    try { evaluator.evaluate(currentFunc, 1.0); }
    catch (exception& e) {
        cout << "  [!] Error en la expresión: " << e.what() << "\n";
        cout << "  Presione Enter para reintentar..."; cin.get();
        leerFuncion();
    }
}

double leerDouble(const string& prompt) {
    double v; cout << "  " << prompt; cin >> v; return v;
}
int leerInt(const string& prompt) {
    int v; cout << "  " << prompt; cin >> v; return v;
}

void encabezado(const string& titulo) {
    cout << "\n" << string(60, '=') << "\n";
    cout << "  " << titulo << "\n";
    cout << string(60, '=') << "\n";
}
void separador() { cout << string(60, '-') << "\n"; }

void pausar() {
    cout << "\n  Presione Enter para continuar...";
    cin.ignore(); cin.get();
}

// ─────────────────────────────────────────────────────────────────────────────
//  3.1 DERIVACIÓN NUMÉRICA
// ─────────────────────────────────────────────────────────────────────────────
void derivacionNumerica() {
    encabezado("3.1 DERIVACIÓN NUMÉRICA");
    cout << "\n  Métodos disponibles:\n";
    cout << "  [1] Diferencia hacia adelante  (O(h))\n";
    cout << "  [2] Diferencia hacia atrás     (O(h))\n";
    cout << "  [3] Diferencia central         (O(h²))\n";
    cout << "  [4] Segunda derivada central   (O(h²))\n";
    cout << "  [5] Derivada de orden superior (Richardson)\n";
    cout << "  [0] Regresar\n";
    int op = leerInt("Opción: ");
    if (op == 0) return;

    leerFuncion();
    double x0 = leerDouble("Punto x₀ donde derivar: ");
    double h  = leerDouble("Tamaño de paso h: ");

    separador();
    cout << fixed << setprecision(10);

    double res = 0;
    string nombre;

    switch(op) {
        case 1:
            res = (f(x0 + h) - f(x0)) / h;
            nombre = "Diferencia hacia adelante f'(x₀)";
            break;
        case 2:
            res = (f(x0) - f(x0 - h)) / h;
            nombre = "Diferencia hacia atrás f'(x₀)";
            break;
        case 3:
            res = (f(x0 + h) - f(x0 - h)) / (2*h);
            nombre = "Diferencia central f'(x₀)";
            break;
        case 4:
            res = (f(x0 + h) - 2*f(x0) + f(x0 - h)) / (h*h);
            nombre = "Segunda derivada central f''(x₀)";
            break;
        case 5: {
            // Extrapolación de Richardson: D(h) y D(h/2)
            double D1 = (f(x0+h)   - f(x0-h))   / (2*h);
            double D2 = (f(x0+h/2) - f(x0-h/2)) / h;
            res = (4*D2 - D1) / 3;
            nombre = "Derivada Richardson O(h⁴) f'(x₀)";
            break;
        }
        default: cout << "  Opción inválida.\n"; pausar(); return;
    }

    cout << "\n  Función : f(x) = " << currentFunc << "\n";
    cout << "  Punto   : x₀ = " << x0 << "\n";
    cout << "  Paso    : h  = " << h  << "\n";
    separador();
    cout << "  " << nombre << " = " << res << "\n";

    // Tabla de convergencia
    cout << "\n  Tabla de convergencia (reduciendo h):\n";
    cout << "  " << left << setw(15) << "h" << setw(22) << "Aprox. derivada" << "Error relativo\n";
    separador();
    double prev = 0;
    for (int i = 0; i <= 8; i++) {
        double hi = h * pow(10, -i);
        double di = (f(x0+hi) - f(x0-hi)) / (2*hi);
        double err = (i>0 && prev!=0) ? fabs((di-prev)/di)*100 : 0;
        cout << "  " << left << setw(15) << hi << setw(22) << di;
        if (i > 0) cout << err << " %";
        cout << "\n";
        prev = di;
    }
    pausar();
}

// ─────────────────────────────────────────────────────────────────────────────
//  3.2.1 REGLA TRAPEZOIDAL
// ─────────────────────────────────────────────────────────────────────────────
void reglaTrapecio() {
    encabezado("3.2.1 REGLA TRAPEZOIDAL");

    leerFuncion();
    double a = leerDouble("Límite inferior a: ");
    double b = leerDouble("Límite superior b: ");
    int n    = leerInt("Número de subintervalos n (1 = simple, n>1 = compuesto): ");

    if (n < 1) { cout << "  n debe ser >= 1\n"; pausar(); return; }

    double h = (b - a) / n;
    double suma = f(a) + f(b);
    for (int i = 1; i < n; i++) suma += 2 * f(a + i*h);
    double resultado = (h / 2.0) * suma;

    separador();
    cout << fixed << setprecision(10);
    cout << "\n  Función  : f(x) = " << currentFunc << "\n";
    cout << "  Intervalo: [" << a << ", " << b << "]\n";
    cout << "  n        : " << n << "  |  h = " << h << "\n";
    separador();
    cout << "  Integral ≈ " << resultado << "\n";

    // Tabla de puntos
    if (n <= 20) {
        cout << "\n  Puntos usados:\n";
        cout << "  " << setw(6) << "i" << setw(18) << "xᵢ" << setw(18) << "f(xᵢ)" << "  coef.\n";
        separador();
        for (int i = 0; i <= n; i++) {
            double xi = a + i*h;
            int coef = (i==0 || i==n) ? 1 : 2;
            cout << "  " << setw(6) << i << setw(18) << xi << setw(18) << f(xi) << "    " << coef << "\n";
        }
    }

    // Error estimado
    cout << "\n  Error de truncamiento: O(h²) = O(" << h*h << ")\n";
    pausar();
}

// ─────────────────────────────────────────────────────────────────────────────
//  3.2.2 REGLA DE SIMPSON 1/3
// ─────────────────────────────────────────────────────────────────────────────
void simpsonUnTercio() {
    encabezado("3.2.2 REGLA DE SIMPSON 1/3");

    leerFuncion();
    double a = leerDouble("Límite inferior a: ");
    double b = leerDouble("Límite superior b: ");
    int n    = leerInt("Número de subintervalos n (debe ser PAR): ");

    if (n < 2 || n % 2 != 0) {
        cout << "  [!] n debe ser par y >= 2. Se ajusta a " << (n + n%2) << "\n";
        n = (n < 2) ? 2 : n + n%2;
    }

    double h = (b - a) / n;
    double suma = f(a) + f(b);
    for (int i = 1; i < n; i++) {
        double xi = a + i*h;
        suma += (i % 2 == 0) ? 2*f(xi) : 4*f(xi);
    }
    double resultado = (h / 3.0) * suma;

    separador();
    cout << fixed << setprecision(10);
    cout << "\n  Función  : f(x) = " << currentFunc << "\n";
    cout << "  Intervalo: [" << a << ", " << b << "]\n";
    cout << "  n        : " << n << "  |  h = " << h << "\n";
    separador();
    cout << "  Integral ≈ " << resultado << "\n";

    if (n <= 20) {
        cout << "\n  Puntos usados:\n";
        cout << "  " << setw(6) << "i" << setw(18) << "xᵢ" << setw(18) << "f(xᵢ)" << "  coef.\n";
        separador();
        for (int i = 0; i <= n; i++) {
            double xi = a + i*h;
            int coef = (i==0||i==n) ? 1 : (i%2==0 ? 2 : 4);
            cout << "  " << setw(6) << i << setw(18) << xi << setw(18) << f(xi) << "    " << coef << "\n";
        }
    }
    cout << "\n  Error de truncamiento: O(h⁴)\n";
    pausar();
}

// ─────────────────────────────────────────────────────────────────────────────
//  3.2.3 REGLA DE SIMPSON 3/8
// ─────────────────────────────────────────────────────────────────────────────
void simpsonTresOctavos() {
    encabezado("3.2.3 REGLA DE SIMPSON 3/8");

    leerFuncion();
    double a = leerDouble("Límite inferior a: ");
    double b = leerDouble("Límite superior b: ");
    int n    = leerInt("Número de subintervalos n (debe ser múltiplo de 3): ");

    if (n < 3 || n % 3 != 0) {
        int adj = n + (3 - n%3) % 3;
        if (adj < 3) adj = 3;
        cout << "  [!] n ajustado a " << adj << "\n";
        n = adj;
    }

    double h = (b - a) / n;
    double suma = f(a) + f(b);
    for (int i = 1; i < n; i++) {
        double xi = a + i*h;
        suma += (i % 3 == 0) ? 2*f(xi) : 3*f(xi);
    }
    double resultado = (3.0*h / 8.0) * suma;

    separador();
    cout << fixed << setprecision(10);
    cout << "\n  Función  : f(x) = " << currentFunc << "\n";
    cout << "  Intervalo: [" << a << ", " << b << "]\n";
    cout << "  n        : " << n << "  |  h = " << h << "\n";
    separador();
    cout << "  Integral ≈ " << resultado << "\n";

    if (n <= 20) {
        cout << "\n  Puntos usados:\n";
        cout << "  " << setw(6) << "i" << setw(18) << "xᵢ" << setw(18) << "f(xᵢ)" << "  coef.\n";
        separador();
        for (int i = 0; i <= n; i++) {
            double xi = a + i*h;
            int coef = (i==0||i==n) ? 1 : (i%3==0 ? 2 : 3);
            cout << "  " << setw(6) << i << setw(18) << xi << setw(18) << f(xi) << "    " << coef << "\n";
        }
    }
    cout << "\n  Error de truncamiento: O(h⁴)\n";
    pausar();
}

// ─────────────────────────────────────────────────────────────────────────────
//  3.3 INTEGRACIÓN DE ROMBERG
// ─────────────────────────────────────────────────────────────────────────────
void romberg() {
    encabezado("3.3 INTEGRACIÓN DE ROMBERG");

    leerFuncion();
    double a   = leerDouble("Límite inferior a: ");
    double b   = leerDouble("Límite superior b: ");
    int maxIter = leerInt("Niveles de refinamiento (filas de la tabla, recomendado 4-8): ");
    double tol  = leerDouble("Tolerancia de convergencia (ej: 1e-6): ");

    if (maxIter < 1) maxIter = 1;
    if (maxIter > 15) { cout << "  [!] Máximo 15 niveles. Ajustado.\n"; maxIter = 15; }

    // R[i][j] = tabla de Romberg
    vector<vector<double>> R(maxIter+1, vector<double>(maxIter+1, 0.0));

    cout << fixed << setprecision(10);
    cout << "\n  Función  : f(x) = " << currentFunc << "\n";
    cout << "  Intervalo: [" << a << ", " << b << "]\n";
    separador();

    // R[0][0] = Trapecio simple
    R[0][0] = (b - a) / 2.0 * (f(a) + f(b));

    int iConv = maxIter;

    for (int i = 1; i <= maxIter; i++) {
        int n = 1 << i; // 2^i subintervalos
        double h = (b - a) / n;

        // Trapecio compuesto con 2^i subintervalos
        double suma = 0;
        for (int k = 1; k <= n/2; k++)
            suma += f(a + (2*k - 1)*h);
        R[i][0] = 0.5 * R[i-1][0] + h * suma;

        // Extrapolación de Richardson
        for (int j = 1; j <= i; j++) {
            double factor = pow(4.0, j);
            R[i][j] = (factor * R[i][j-1] - R[i-1][j-1]) / (factor - 1.0);
        }

        // Verificar convergencia
        if (i >= 1 && fabs(R[i][i] - R[i-1][i-1]) < tol) {
            iConv = i;
            break;
        }
    }

    // Imprimir tabla
    cout << "\n  TABLA DE ROMBERG:\n\n";
    cout << "  " << left << setw(6) << "i/j";
    for (int j = 0; j <= iConv; j++)
        cout << right << setw(16) << ("O(h^" + to_string((int)pow(2, j+1)) + ")");
    cout << "\n";
    separador();

    for (int i = 0; i <= iConv; i++) {
        cout << "  " << left << setw(6) << i;
        for (int j = 0; j <= i; j++)
            cout << right << setw(16) << R[i][j];
        cout << "\n";
    }

    separador();
    cout << "\n  Resultado final R[" << iConv << "][" << iConv << "] = " << R[iConv][iConv] << "\n";

    if (iConv < maxIter)
        cout << "  Convergencia alcanzada en nivel " << iConv << " (tolerancia " << tol << ")\n";
    else
        cout << "  Niveles máximos alcanzados (puede aumentar niveles o reducir tolerancia)\n";

    cout << "  Orden de precisión: O(h^" << (int)pow(2, iConv+1) << ")\n";
    pausar();
}

// ─────────────────────────────────────────────────────────────────────────────
//  MENÚ NEWTON-COTES
// ─────────────────────────────────────────────────────────────────────────────
void menuNewtonCotes() {
    int op;
    do {
        encabezado("3.2 FÓRMULAS DE NEWTON-COTES");
        cout << "\n  [1] Regla Trapezoidal\n";
        cout << "  [2] Regla de Simpson 1/3\n";
        cout << "  [3] Regla de Simpson 3/8\n";
        cout << "  [0] Regresar al menú principal\n";
        op = leerInt("Opción: ");
        switch(op) {
            case 1: reglaTrapecio();    break;
            case 2: simpsonUnTercio();  break;
            case 3: simpsonTresOctavos(); break;
            case 0: break;
            default: cout << "  Opción inválida.\n";
        }
    } while (op != 0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  MENÚ PRINCIPAL
// ─────────────────────────────────────────────────────────────────────────────
void mostrarBienvenida() {
    cout << "\n";
    cout << "  ╔══════════════════════════════════════════════════════════╗\n";
    cout << "  ║        MÉTODOS NUMÉRICOS — DERIVACIÓN E INTEGRACIÓN      ║\n";
    cout << "  ║                   Unidad 3                               ║\n";
    cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    cout << "\n  Temas disponibles:\n";
    cout << "    3.1  Derivación Numérica\n";
    cout << "    3.2  Newton-Cotes (Trapecio, Simpson 1/3, Simpson 3/8)\n";
    cout << "    3.3  Integración de Romberg\n\n";
}

int main() {
    mostrarBienvenida();

    int op;
    do {
        encabezado("MENÚ PRINCIPAL");
        cout << "\n  [1] 3.1  Derivación Numérica\n";
        cout << "  [2] 3.2  Newton-Cotes\n";
        cout << "  [3] 3.3  Integración de Romberg\n";
        cout << "  [0] Salir\n";
        op = leerInt("Opción: ");
        switch(op) {
            case 1: derivacionNumerica(); break;
            case 2: menuNewtonCotes();    break;
            case 3: romberg();            break;
            case 0: cout << "\n  ¡Hasta luego!\n\n"; break;
            default: cout << "  Opción inválida.\n";
        }
    } while (op != 0);

    return 0;
}
