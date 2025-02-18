/*
Federico Teran
Santiago Arango
Oscar Vargas

Se implementa ADD, SET, LDR, STR, INC, DEC, SHW, PAUSE, END
Notar que se asume que cada instruccion toma 3 argumentos (los no usados son NULL)
Notar que las instrucciones no entran al modelo por la MDR (se decidio hacerlo aparte para no tener que decodificar un numero)

Clases:
	- Memory      : Implementa la memoria como un arreglo
	- Register    : Implementa un registro como un entero
	- CPU         : Maneja sus registros, decodifica, ejecuta las instrucciones
	- IO_man      : es la interfaz del CPU para acceder a la memoria, las
						instrucciones y stdout
	- Instruction : Almacena la informacion relacionada a 1 instruccion
Funciones:
	- load_data : Lee las instrucciones de stdin, las transforma a tipo 'Instruction'
					y las almacena en un std::vector<Instruction>

*/
#include <iostream>
#include <string>
#include <assert.h>

#define MemoryLimit 100

#define my_null -100
#define shw_acc -1
#define shw_icr -2
#define shw_mar -3
#define shw_mdr -4
#define shw_uc -5

class Memory; class Register; class CPU; class IO_man; class Instruction;

class Instruction{
	private:
	std::string the_string;
	static int num_decode( const std::string &st ){
		int res =0; // extrae el numero de st
		for ( int i = (st[0]=='D')?1:0 ; i < int(st.size()) ; ++i ) {
			res = res*10 + st[i]-'0';
		}
		return res;
	}
	public:
	std::string type;
	int arg1, arg2, arg3;
	Instruction(){
		this->type = my_null; this->arg1=my_null; this->arg2=my_null; this->arg3=my_null;
	}
	Instruction( const std::string &tp, const std::string &a1, const std::string &a2, const std::string &a3 ){
		this->type = tp; // el tipo de la instruccion
		// cargo el primer argumento
		if ( a1 == "NULL" ) this->arg1 = my_null;
		else if ( tp == "SHW" && a1[0]!='D' ) {
			// shw puede tener ACC, ICR, MAR, MDR o UC
			if ( a1 == "ACC" ) {
				this->arg1 = shw_acc;
			} else if ( a1 == "ICR" ) {
				this->arg1 = shw_icr;
			} else if ( a1 == "MAR" ) {
				this->arg1 = shw_mar;
			} else if ( a1 == "MDR" ) {
				this->arg1 = shw_mdr;
			} else if ( a1 == "UC" ) {
				this->arg1 = shw_uc;
			} else assert( false );
		} else this->arg1 = Instruction::num_decode( a1 );
		// cargo el segundo argumento
		if ( a2 == "NULL" ) this->arg2 = my_null;
		else this->arg2 = Instruction::num_decode( a2 );
		// cargo el tercer argumento
		if ( a3 == "NULL" ) this->arg3 = my_null;
		else this->arg3 = Instruction::num_decode( a3 );
		// para imprimir
		this->the_string= tp + " " + a1 + " " + a2 + " " + a3;
	}
	
	std::string stringify(){
		return this->the_string;
	}
};

class Memory{
	private:
	int mem[MemoryLimit];
	int memory_size;
	Register *MAR, *MDR;
	public:
	Memory()=default;
	Memory( int sz ) {
		this->memory_size=sz;
	}
	int read( int pos ) {
		assert( pos < this->memory_size );
		return this->mem[pos];
	}
	void write( int pos, int val ) {
		assert( pos < this->memory_size );
		this->mem[pos] = val;
	}
	int size(){
		return this->memory_size;
	}
	
};

class Register{
	private:
	int val;
	public:
	Register( int v=0 ) : val(v) {};
	int read() {
		return this->val;
	}
	void write( int val ) {
		this->val = val;
	}
};

class IO_man{
	private:
	Instruction *inst;
	Memory *mem;
	Register *MDR, *MAR;
	public:
	IO_man()=default;
	IO_man( Memory *mem, Register *mdr, Register *mar, Instruction *in ){
		// MDR y MAR lo acceden tanto IO_man como CPU
		this->mem=mem; this->MDR = mdr; this->MAR = mar;
		this->inst = in;
	}
	Instruction inst_read(){
		return this->inst[this->MAR->read()];
	}
	int read(){
		this->MDR->write( this->mem->read( this->MAR->read() ) );
	}
	void write() {
		this->mem->write( this->MAR->read(), this->MDR->read() );
	}
	void show( int dat, int type ) {
		if ( type >= 0 ) {
			// es una direccion de memoria y dat==-1
			dat = this->mem->read( type );
		}
		if ( type > shw_uc ) {
			// en este caso dat ya tiene lo que se debe imprimir
			std::cout << dat << std::endl;
		} else {
			// en este caso dat tiene la instruccion a imprimir
			// SHW UC NULL NULL
			std::cout << this->inst[dat].stringify() << std::endl;
		}
	}
	void pause( int acc, int icr, int mar, int mdr ){
		std::cout << "Inicio pausa" << std::endl;
		std::cout << "ACC: " << acc << ". ICR: " << icr << ". MAR: " << mar << ". MDR: " << mdr << std::endl;
		std::cout << "Instruccion en Control Unit: "<< this->inst[icr-1].stringify() << std::endl; // porque icr:=icr+1 en el fetch
		std::cout << "En memoria:" << std::endl;
		for ( int i = 0 ; i < this->mem->size() ; ++i ) {
			std::cout << "[" << i << "] -> " << this->mem->read(i) << " | ";
		}
		std::cout << std::endl << "Fin Pausa " << std::endl << std::endl;
		
	}
};

class CPU{
	private:
	IO_man *IO;
	Register *MAR, *MDR, *ICR, *ACC;
	Instruction control_unit;
	
	void fetch() {
		this->MAR->write( this->ICR->read() ); // para que pueda leer la inst
		control_unit = this->IO->inst_read(); // la instruccion entra a UC
		this->ICR->write( this->ICR->read()+1 ); // icr:=icr+1
	}
	bool execute() {
		bool end = false;
		if ( control_unit.type == "SET" ) {
			// arg1 tiene la direccion, arg2 tiene el valor
			this->MAR->write(control_unit.arg1);
			this->MDR->write(control_unit.arg2);
			this->IO->write(); // escribo en memoria
		} else if ( control_unit.type == "LDR" ) {
			this->MAR->write(control_unit.arg1);
			this->IO->read(); // me deja la data en MDR
			this->ACC->write( this->MDR->read() ); // lo cargo a ACC
		} else if ( control_unit.type == "ADD" ) {
			this->MAR->write( control_unit.arg1 );
			this->IO->read(); // leo de arg1
			int sum = this->MDR->read();
			if ( control_unit.arg2 == my_null ) {
				sum += this->ACC->read(); // ADD D? NULL NULL
			} else {
				this->MAR->write( control_unit.arg2 );
				this->IO->read(); // leo de arg2
				sum += this->MDR->read(); // sumo los dos datos
			}
			this->ACC->write( sum ); // dejo en ACC el resultado
			if ( control_unit.arg3 != my_null ) {
				this->MAR->write( control_unit.arg3 );
				this->MDR->write( this->ACC->read() );
				this->IO->write(); // escribo en arg3
			}
		} else if ( control_unit.type == "INC" ) {
			this->MAR->write( control_unit.arg1 );
			this->IO->read(); // leo de arg1
			this->ACC->write( this->MDR->read() + 1 ); // incremento
			this->MDR->write( this->ACC->read() ); 
			this->IO->write(); // guardo de nuevo en arg1
	    } else if ( control_unit.type == "DEC" ) {
			this->MAR->write( control_unit.arg1 );
			this->IO->read(); // leo de arg1
			this->ACC->write( this->MDR->read() - 1 ); // decremento
			this->MDR->write( this->ACC->read() );
			this->IO->write(); // guardo de nuevo en arg1
		} else if ( control_unit.type == "STR" ) {
			this->MAR->write( control_unit.arg1 );
			this->MDR->write( this->ACC->read() );
			this->IO->write(); // escribo en arg1 lo de ACC
		} else if ( control_unit.type == "SHW" ) {
			int dat=-1;
			// le mando el dato correspondiente
			if ( control_unit.arg1 == shw_acc ) {
				dat = this->ACC->read();
			} else if ( control_unit.arg1 == shw_icr ) {
				dat = this->ICR->read();
			} else if ( control_unit.arg1 == shw_mar ) {
				dat = this->MAR->read();
			} else if ( control_unit.arg1 == shw_mdr ) {
				dat = this->MDR->read();
			} else if ( control_unit.arg1 == shw_uc ) {
				dat = this->ICR->read()-1; // no quiero mandar asi nomas la instr
			} 
			this->IO->show(dat, control_unit.arg1 ); // invoco IO_man
		} else if ( control_unit.type == "PAUSE" ) {
			// envio los datos de mis registros a IO_man
			this->IO->pause( 
				this->ACC->read(),
				this->ICR->read(),
				this->MAR->read(),
				this->MDR->read() );
		} else if ( control_unit.type == "END" ) {
			end=true; // termino ejecucion
		} else assert(false);
		return end;
	}
	
	void instruction_cycle(){
		// Nota: en 'execute' tambien esta el decodificado de la instruccion
		do {
			fetch();
		} while ( !execute() );
	}
	
	public:
	CPU()=default;
	CPU( IO_man *io, Register *mar, Register *mdr ) {
		this->MAR = mar; this->MDR = mdr;
		this->IO = io;
		this->ICR = new Register(); this->ACC = new Register();
	}
	~CPU(){
		delete this->ICR;
		delete this->ACC;
	}
	void start(){
		this->ICR->write( 0 ); // inicio en la instruccion 0
		instruction_cycle();
	}
};

Instruction * load_data(){
	Instruction *inst = new Instruction[MemoryLimit];
	std::string type, arg1, arg2, arg3;
	int cnt =0;
	while ( std::cin >> type ) {
		std::cin >> arg1 >> arg2 >> arg3;
		inst[cnt] = Instruction( type, arg1, arg2, arg3 );
		++cnt;
	}
	return inst;
}

int main(){
	int memory_size=10; // el tamaño de la memoria
	Instruction *data = load_data(); // cargo las instrucciones
	
	// para debug
	// for ( const Instruction &a : data ) std::cout << a.type << " _ " << a.arg1 << " ; " << a.arg2 << std::endl;
	
	// instanciando todos los elementos
	Memory * mem = new Memory( memory_size );
	Register *MAR = new Register(), *MDR = new Register();
	IO_man *IO = new IO_man( mem, MDR, MAR, data );
	CPU *procesador = new CPU( IO, MAR, MDR );
	
	procesador->start();
	
	// liberando la memoria
	delete procesador;
	delete IO;
	delete MAR; delete MDR;
	delete mem;
	delete data;
	
	return 0;
}
