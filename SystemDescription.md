# Opis sistema:

## Opis problema:

	Implementiranje "primitivne" apstrakte interpretacije korišćenjem ugrađene biblioteke
	ConstantRange.h, koju daje LLVM platforma, za analizu intervala celobrojnih promenljivih
	do dostizanja fiksne tačke programa. Program napisan na C++ jeziku se analizira nakon 
	prevođenja na međukod LLVM platforme (LLVM IR). Cilj je praćenje izvršavanje programa,
	nakon podele istog na bazične blokove, kao i adekvatno menjanje intervala celobrojnih promeljivih,
	u zavisnosti od instrukcija nad njima, tokom rada programa. Ispis postaje kompleksan za programe
	sa više poziva funkcija, te se posmatraju bazičniji primeri.

## Opis arhitekture sistema (opis osnovnih modula implementacije):
  
* CMakeLists.txt:
	Potreban za alat cmake, sadrži potrebne parametre za prevođenje programa
* ai_intervali:
	* enum class VredResetke : char:
		Definiše rešetku.
	* class AIProlaz : public FunctionPass:
		Klasa koja definiše LLVM prolaz (LLVM Pass), glavna klasa.
	* bool runOnFunction(Function& F) override:
		Funkcija koja sve pokreće, definisana je u LLVM biblioteci te mora override.
	* std::pair<VredResetke, ConstantRange*> _obradjenaVred(Value* vred):
		Funkcija koja obrađuje elemente programa i u zavisnosti od tipa dodeljuje adekvatne vrednosti rešetke.
	* std::pair<VredResetke, ConstantRange*> funk_tran(Value* vred):
		Funkcija koja definiše funkcije transfera koje obezbeđuju odgovarajuće menjanje intervala u zavisnosti od instrukcije programa. Ovde je suština programa, u zavisnosti od broja obrađenih instrukcija program je kompletniji.
	* std::pair<VredResetke, ConstantRange*> vrati_trenutnu_vred(Value* vred):
		Funkcija koja vraća trenutnu vrednost rešetke i interval za prosleđen element za dalju obradu.
	* std::queue<Value*> init(Function& F):
		Funkcija koja vrši inicijalizaciju celokupnog programa. Kreće se kroz bazne blokove programa, kroz njihove instrukcije i inicijalzuje svakom elementu odgovarajući interval koji definiše.
	* void dumpAnalysis():
		Funkcija koja vrši ispis analize i koristi stream outs() definisan od strane LLVM-a. Bolji ispis se može postići korišćenjem DEBUG opcija (koje nažalost nismo mogli da implementiramo iz nama nepoznatih razloga).
	* static RegisterPass<AIProlaz> X("AI-PROLAZ", "Apstraktna interpretacija", false, false):
		Funkcija koja registruje LLVM prolaz tako da se on zada kao opcija LLVM alatu. Predefinisana od strane LLVM-a.

## Opis rešenja problema (osnovne ideje, obrazloženja za ključne odluke, opis osnovnog algoritma):

* Za pravilno prevođenje i linkovanje korišćen je alat **cmake** zajedno sa **make** alatom koji ceo proces automatizuju.
* Program je pisan kao LLVM prolaz (pass) jer predstavlja najelegantniji način implementiranja algoritama apstraktne interpretacije ali i algoritama za statičku analizu programa uopšte.
* Određen broj korišćenih funkcija je imala definisanu funkcionalnost od strane LLVM-a koju je trebalo prilagoditi ili nadograditi u svrhe ispunjenja cilja.
* Struktura programa prati (osim dela sa debug ispisom) LLVM-ovu dokumentaciju za pisanje LLVM prolaza datu na linku **http://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass**.
* Teorijska ideja programa je zasnovana na osnovu poznate teorije rađene na predavanjima (**http://www.verifikacijasoftvera.matf.bg.ac.rs/vs/predavanja/08_abstraktna_interpretacija/29_DimitrijeSpadijer_ApstraktnaInterpretacija.pdf**).
* Ideja programa je da se na osnovu grafa kontrole podataka, implicitno kreiranog od strane LLVM-a, prođe kroz sve bazne blokove programa, samim tim i kroz sve instrukcije baznih blokova, zatim svakoj od promenljivih dodeli odgovarajuća vrednost rešetke (tj. intervala) koju definiše i zatim da se u skladu sa operacijama koje se nad tim promenljivama izvršavaju menjaju krajevi tih intervala.
* Mape i torke se često koriste u programu kao efikasan način čuvanja podataka i promenljivih.
* Cilj je bio iskoristiti LLVM platformu, zajedno sa obezbeđenim bibliotekama, zarad implementacije nekog algoritma apstraktne interpretacije.

