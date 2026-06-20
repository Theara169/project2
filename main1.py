from flask import Flask, jsonify, request, send_from_directory, session, redirect, url_for
import sqlite3
import os
import traceback

app = Flask(__name__, static_folder='.', static_url_path='')
app.secret_key = 'your-secret-key-here'  # CHANGE THIS!

# ---- Use absolute path for database ----
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATABASE = os.path.join(BASE_DIR, 'employee1.db')

def get_db():
    conn = sqlite3.connect(DATABASE)
    conn.row_factory = sqlite3.Row
    return conn

# ---- Create employee table if it doesn't exist ----
def init_db():
    try:
        with get_db() as conn:
            conn.execute('''
                CREATE TABLE IF NOT EXISTS employee (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT NOT NULL,
                    age INTEGER NOT NULL,
                    position TEXT NOT NULL,
                    salary REAL NOT NULL,
                    email TEXT UNIQUE NOT NULL,
                    password TEXT NOT NULL
                )
            ''')
            conn.commit()
            print("Database initialized successfully at:", DATABASE)
    except Exception as e:
        print("ERROR initializing database:", e)
        traceback.print_exc()

init_db()

# ---- Authentication Routes ----
@app.route('/login')
def login_page():
    if 'user_id' in session:
        return redirect(url_for('dashboard'))
    return send_from_directory('.', 'login.html')

@app.route('/register')
def register_page():
    if 'user_id' in session:
        return redirect(url_for('dashboard'))
    return send_from_directory('.', 'createAccount.html')

@app.route('/api/register', methods=['POST'])
def register():
    try:
        data = request.get_json()
        if not data:
            return jsonify({'error': 'No JSON data received'}), 400

        name = data.get('name')
        age = data.get('age')
        position = data.get('position')
        salary = data.get('salary')
        email = data.get('email')
        password = data.get('password')

        # Validate all fields present
        if not all([name, age, position, salary, email, password]):
            return jsonify({'error': 'All fields are required'}), 400

        # Validate age
        try:
            age = int(age)
            if age < 18 or age > 100:
                return jsonify({'error': 'Age must be between 18 and 100'}), 400
        except ValueError:
            return jsonify({'error': 'Age must be a number'}), 400

        # Validate salary
        try:
            salary = float(salary)
            if salary < 0:
                return jsonify({'error': 'Salary must be positive'}), 400
        except ValueError:
            return jsonify({'error': 'Salary must be a number'}), 400

        conn = get_db()
        # Check if name or email already exists
        existing = conn.execute('SELECT id FROM employee WHERE name = ? OR email = ?', (name, email)).fetchone()
        if existing:
            conn.close()
            return jsonify({'error': 'Employee with that name or email already exists'}), 409

        # Insert new employee
        conn.execute(
            'INSERT INTO employee (name, age, position, salary, email, password) VALUES (?, ?, ?, ?, ?, ?)',
            (name, age, position, salary, email, password)
        )
        conn.commit()
        conn.close()
        return jsonify({'message': 'Employee registered successfully'}), 201

    except Exception as e:
        print("Registration error:", e)
        traceback.print_exc()
        return jsonify({'error': f'Server error: {str(e)}'}), 500

@app.route('/api/login', methods=['POST'])
def login():
    try:
        data = request.get_json()
        if not data:
            return jsonify({'error': 'No JSON data received'}), 400

        name = data.get('username')
        password = data.get('password')
        if not name or not password:
            return jsonify({'error': 'Name and password required'}), 400

        conn = get_db()
        employee = conn.execute('SELECT id, name, password FROM employee WHERE name = ?', (name,)).fetchone()
        conn.close()
        if not employee or employee['password'] != password:
            return jsonify({'error': 'Invalid credentials'}), 401

        session['user_id'] = employee['id']
        session['username'] = employee['name']
        return jsonify({'message': 'Login successful'}), 200

    except Exception as e:
        print("Login error:", e)
        traceback.print_exc()
        return jsonify({'error': f'Server error: {str(e)}'}), 500

@app.route('/api/logout', methods=['POST'])
def logout():
    session.clear()
    return jsonify({'message': 'Logged out'}), 200

# ---- Protected Dashboard ----
@app.route('/')
def dashboard():
    if 'user_id' not in session:
        return redirect(url_for('login_page'))
    return send_from_directory('.', 'index.html')

# ---- API endpoints (protected) ----
@app.route('/api/employees')
def get_all():
    if 'user_id' not in session:
        return jsonify({'error': 'Unauthorized'}), 401
    try:
        conn = get_db()
        rows = conn.execute('SELECT id, name, age, position, salary, email FROM employee').fetchall()
        conn.close()
        return jsonify([dict(row) for row in rows])
    except Exception as e:
        print("Error fetching employees:", e)
        return jsonify({'error': str(e)}), 500

@app.route('/api/search')
def search():
    if 'user_id' not in session:
        return jsonify({'error': 'Unauthorized'}), 401
    name = request.args.get('name', '').strip()
    if not name:
        return jsonify([])
    try:
        conn = get_db()
        rows = conn.execute(
            "SELECT id, name, age, position, salary, email FROM employee WHERE name LIKE ?",
            (f'%{name}%',)
        ).fetchall()
        conn.close()
        return jsonify([dict(row) for row in rows])
    except Exception as e:
        print("Search error:", e)
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True, port=5000)