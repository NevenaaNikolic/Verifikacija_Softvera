#include <unordered_map>
#include <queue>

#include "llvm/IR/Constants.h"  // dodato zbog class llvm::ConstantInt
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Definisemo nasu resetku
enum class VredResetke : char
{
	// Ne koristimo Vrh jer ce interval biti ogranicen svojom sirinom
	// Inace bi mogli da koristimo inicijalno MAX_INT (samo sa celim brojevima radimo)
    ConstantRange,
    Dno,
};

class AIProlaz : public FunctionPass
{
public:
    static char ID;
    AIProlaz() : FunctionPass(ID){}
	
	// Override-ujemo metod koji je vec definisan u llvm-u, ovde je sustina programa
    bool runOnFunction(Function& F) override
	{
		// Inicijalizujemo listu intervala na osnovu prosledjene funkcije
		std::queue<Value*> lista_intervala = init(F);
		
		while (!lista_intervala.empty())
		{
			// Uzimamo jednu vrednost i izbacujemo iz liste
			Value* vred = lista_intervala.front();
			lista_intervala.pop();
			
			std::pair<VredResetke, ConstantRange*> stara_vred = vrati_trenutnu_vred(vred);
			std::pair<VredResetke, ConstantRange*> nova_vred = _obradjenaVred(vred);
			
			// Azuriranje vrednosti
			if (stara_vred.second != nova_vred.second)
			{
				// Brisemo, ako postoje, dosadasnji konstantni intervali
				if (_mapaIntervala.find(vred) != _mapaIntervala.find(vred))
				{
					if (_mapaIntervala.at(vred) != nullptr)
					{
						delete _mapaIntervala.at(vred);
					}
				}
				
				// Azuriranje mapa
				_mapaVrednosti[vred] = nova_vred.first;
				_mapaIntervala[vred] = nova_vred.second;
				
				for (auto korisnik : vred->users())
				{
					lista_intervala.push(cast<Value>(korisnik));
				}
			}
		}
		
		// Metod za ispis rezultata
		dumpAnalysis();
		
		return false;
   }
private:
    // Vrednosti resetke (Lattice Values)
    std::unordered_map<Value*, VredResetke> _mapaVrednosti;
	
	// Ako je u mapi gore sacuvana vrednost tipa konstantnog intervala
	// onda se taj konstantni interval cuva bas u ovoj dole mapi
	// Mape su povezane preko svojih prvih elemenata
    std::unordered_map<Value*, ConstantRange*> _mapaIntervala;
	
    std::pair<VredResetke, ConstantRange*> _obradjenaVred(Value* vred)
	{
		std::pair<VredResetke, ConstantRange*> _rezultat;
		
		// Radimo samo sa celobrojnim tipom
		// Program se moze unaprediti za rad sa vise tipova
		
		// Obrada necelobrojnih tipova
		if (!vred->getType()-> isIntegerTy())
			return {VredResetke::Dno, nullptr};
		
		// Sirina odredjena velicinom celog broja, daje nam gornju granicu intervala
		unsigned sirina = vred->getType() -> getIntegerBitWidth();
		
		// LLVM-ov nacin ispisa
		outs() << "Obradjena vrednost je [sirina]: " << sirina << "\n";
		
		if (auto *funk_arg = dyn_cast<Argument>(vred))
		{
			outs() << "Obradjena vrednost je [argument funkcije]" << "\n";
			outs() << "Ime funkcije: " << funk_arg->getParent() << "\n";
			
			return {VredResetke::ConstantRange, new ConstantRange(sirina, true)};
		}
		
		else if (auto* konst_int = dyn_cast<ConstantInt>(vred))
		{
			outs() << "Obradjena vrednost je [konstantan ceo broj]" << "\n";
			
			return {VredResetke::ConstantRange, new ConstantRange(konst_int->getValue())};
		}
		
		else if (auto* poziv_funk = dyn_cast<CallInst>(vred)) {
			outs() << "Obradjena vrednost je [poziv funkcije]" << "\n";
			return {VredResetke::ConstantRange, new ConstantRange(sirina, true)};
		}
		// Ovo posebno obradjujemo jer se menja interval pri operacijama
		else if (auto* inst = dyn_cast<Instruction>(vred))
		{
			outs() << "Obradjena vrednost je [instrukcija]" << "\n";
			
			return funk_tran (vred);
		}
    }

    // Funkcije transfera za ucitane vrednosti, vraca par
    std::pair<VredResetke, ConstantRange*> funk_tran(Value* vred)
	{
		Instruction* inst = cast<Instruction>(vred);
		assert(inst);
		
		std::pair<VredResetke, ConstantRange*> bazni_par = {VredResetke::Dno, nullptr};
		
		// Trivijalan slucaj
		if (!vred->getType() -> isIntegerTy())
			return bazni_par;
		
		// Program se moze poboljsati i prosiriti funkcijama za obradu i unarnih operacija
		// Slucaj kada nije binarna operacija
		if (!inst->isBinaryOp())
			return bazni_par;
		
		// Slucaj kad je binarna operacija, izdvajamo operande i uzimamo im trenutne vrednosti
		const unsigned sirina = vred->getType()->getIntegerBitWidth();
		Value* v1 = inst->getOperand(0);
		Value* v2 = inst->getOperand(1);
		auto res1 = vrati_trenutnu_vred(v1);
		auto res2 = vrati_trenutnu_vred(v2);
		
		if (res1.first == VredResetke::Dno || res2.first == VredResetke::Dno)
			return bazni_par;
		
		const ConstantRange& r1 = *res1.second;
		const ConstantRange& r2 = *res2.second;
		ConstantRange _rezultat (sirina, true);
		
		// Posmatramo o kojoj operaciji se radi
		switch(inst->getOpcode())
		{
			// Ako je sabiranje
			case Instruction::Add:
				_rezultat = r1.add (r2);
				return {VredResetke::ConstantRange, new ConstantRange(_rezultat)};
				break;
				
			// Ako je oduzimanje
			case Instruction::Sub:
				_rezultat = r1.sub(r2);
				return {VredResetke::ConstantRange, new ConstantRange(_rezultat)};
				break;

			// Ako je mnozenje: novo
			case Instruction::Mul:
				_rezultat = r1.multiply(r2);  // definisana u 890 liniji "ConstantRange.cpp" fajla
			  return {VredResetke::ConstantRange, new ConstantRange(_rezultat)};
				break;

			// Ako nije +,-,*
			default:
				return bazni_par;
		}
    }

    std::pair<VredResetke, ConstantRange*> vrati_trenutnu_vred(Value* vred)
	{
		if (auto* konst_int = dyn_cast<ConstantInt>(vred))
		{
			outs() << "Vracanje trenutne vrednosti: konstantan ceo broj" << "\n";
			_mapaVrednosti[vred] = VredResetke::ConstantRange;
			_mapaIntervala[vred] = new ConstantRange(konst_int->getValue());
		}

		assert(_mapaVrednosti.find(vred) != _mapaVrednosti.end());
		assert(_mapaIntervala.find(vred) != _mapaIntervala.end());

		return {_mapaVrednosti.at(vred), _mapaIntervala.at(vred)};
	}
	
	// Inicijalizacija svih promenljivih i dodela adekvatnih elemenata resetke (lattice elements)
	std::queue<Value*> init(Function& F)
	{
		// Ako nesto ne poznajemo tj. to je promenljiva, stavljamo mu punu vrednost
		// Tj. vrednost intervala koju kao tip definise
		// Inace ga posmatramo kao konstantu
		auto fnIterator = F.arg_begin();
		
		while (fnIterator != F.arg_end())
		{
			Value* arg = fnIterator;
			
			// Krecemo se kroz elemente programa, inicijalizujemo ih i cuvamo
			std::pair<VredResetke, ConstantRange*> res = _obradjenaVred(arg);
			_mapaVrednosti[arg] = res.first;
			_mapaIntervala[arg] = res.second;
			
			fnIterator++;
		}
		
		std::queue<Value*> lista_intervala;
		outs() << "\nInicijalizacija\n\n";
		
		// BB je skraceno za bazni blok, od baznih blokova se sastoji cfg
		// F je skraceno za funkciju
		for (auto& BB : F)
		{
			// I je instrukcija
			for (auto& I : BB)
			{
				// Ispisujemo instrukciju programa u IR obliku
				outs() << I << "\n";
				Value* vred = cast<Value>(&I);
				
				std::pair<VredResetke, ConstantRange*> res = _obradjenaVred(vred);
				_mapaVrednosti[vred] = res.first;
				
				// Brisanje postojecih konstantnih intervala
				if (_mapaIntervala.find(vred) != _mapaIntervala.end())
				{
					if (_mapaIntervala.at(vred) != nullptr)
					delete _mapaIntervala.at(vred);
				}
				
				_mapaIntervala[vred] = res.second;
				lista_intervala.push(vred);
			}
		}
		
		return lista_intervala;
    }
	
    void dumpAnalysis()
	{
		outs() << "\nIspis analize\n";
		outs() << "========================\n";
		
		for (auto& vred : _mapaIntervala)
		{

			if (!isa<Instruction>(vred.first))
				continue;
		
			outs() << *cast<Instruction>(vred.first) << "\n";

			if (!vred.second)
			{
				outs() << "Nema rezultata\n";
				continue;
			}
			
			else if (auto sele = vred.second->getSingleElement())
				outs() << "\tRezultat: " << *sele << "\n";
			
			else
				outs() << "\tRezultat: " << *vred.second << "\n";
		}
    }
};

char AIProlaz::ID = 42;

// Registrovanje prolaza da bi ga kompilator uhvatio
static RegisterPass<AIProlaz> X("AI-PROLAZ", "Apstraktna interpretacija", false, false);
