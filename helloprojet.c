#include <io.h>
#include <inttypes.h>
#include <avr/eeprom.h>


//------------------------------------------------
// Programme "hello world" pour carte à puce
//
//------------------------------------------------


// déclaration des fonctions d'entrée/sortie définies dans "io.c"
void sendbytet0(uint8_t b);
uint8_t recbytet0(void);
uint8_t x;


// variables globales en static ram
uint8_t cla, ins, p1, p2, p3;	// entête de commande
uint8_t sw1, sw2;		// status word

//---------------------------------

uint16_t ee_solde EEMEM;
uint16_t ee_x EEMEM;



//uint8_t y = eeprom_read_byte(&ee_x);
//eeprom_read_byte(src);
//eeprom_write_byte(dest, src);


int taille;		// taille des données introduites -- est initialisé à 0 avant la boucle
#define MAXI 128	// taille maxi des données lues
uint8_t data[MAXI];	// données introduites



// Procédure qui renvoie l'ATR
void atr(uint8_t n, char* hist)
{
    	sendbytet0(0x3b);	// définition du protocole
    	sendbytet0(n);		// nombre d'octets d'historique
    	while(n--)		// Boucle d'envoi des octets d'historique
    	{
        	sendbytet0(*hist++);
    	}
}

void sortir_data()
{

    	// vérification de la taille
    	if (p3!=taille)
    	{
        	sw1=0x6c;	// taille incorrecte
        	sw2=taille;		// taille attendue
        	return;
    	}
	sendbytet0(ins);	// acquittement
	// émission des données
	//int i;
	//for(i=0;i<taille;i++)
    	//{
        //	sendbytet0(data[i]);
    	//}

		uint8_t y = eeprom_read_byte(&ee_x);

    	sw1=0x90;

}

// émission de la version
// t est la taille de la chaîne sv
void version(int t, char* sv)
{
    	int i;
    	// vérification de la taille
    	if (p3!=t)
    	{
        	sw1=0x6c;	// taille incorrecte
        	sw2=t;		// taille attendue
        	return;
    	}
	sendbytet0(ins);	// acquittement
	// émission des données
	for(i=0;i<p3;i++)
    	{
        	sendbytet0(sv[i]);
    	}
    	sw1=0x90;
}


// commande de réception de données
void intro_data()
{
    	int i;
     	// vérification de la taille
    	if (p3!=1)
	{
	   	sw1=0x6c;	// P3 incorrect
        	sw2=MAXI;	// sw2 contient l'information de la taille correcte
		return;
    	}
	sendbytet0(ins);	// acquitement

	//for(i=0;i<p3;i++)	// boucle d'envoi du message
	//{
	    //data[i]=recbytet0();
	//}
	//taille=p3; 		// mémorisation de la taille des données lues
	//sw1=0x90;

		uint8_t x=recbytet0();
		eeprom_write_block(&ee_x,&x,2);
		sw1 = 0x90;

}
void crediter ()

{
	
	if(p3!=2)   //2octets
	{
	sw1=0x6c;
	sw2=2;  //taille 2 octets
	return;
	}
	sendbytet0(ins);
	
	uint8_t x1=recbytet0();
	uint8_t x2=recbytet0();
	uint16_t solde;
	uint16_t somme;


	somme= (x1<<8) + x2; //big endian
	eeprom_read_block(&solde, &ee_solde, 2);
	
	
	if (solde > solde + somme)    //crediter
	{   sw1=0x6c; //taille incorrecte
		sw2=2;
		return;
	}
	else
	{
        solde += somme; 
        eeprom_write_block(&solde, &ee_solde, 2);
        sw1=0x90;
        sw2=00;
	}
}
void debiter ()

{
	
	if(p3!=2)   //2octets
	{
	sw1=0x6c;
		sw2=2;  //taille 2 octets
	return;
	}
	sendbytet0(ins); //acquittement
	
	uint8_t x1=recbytet0();
	uint8_t x2=recbytet0();
	uint16_t solde;
	uint16_t somme;


	somme= (x1<<8) + x2; //big endian
	eeprom_read_block(&solde, &ee_solde, 2);
	
	
    if (solde < solde - somme)  //debiter
    {
        sw1=0x6c; //taille incorrecte
	sw2=00;  //taille attendue
	return;

    }
        else
	{
        solde -= somme; 
        eeprom_write_block(&solde, &ee_solde, 2);
        sw1=0x90;
        sw2=00;
	}
}

void sortir_solde()
{
	if(p3!=2)   //2octets
	{
	sw1=0x6c;
	sw2=2;  //taille 2 octets
	return;
	}
	sendbytet0(ins); //acquittement
	uint8_t solde [2];
	eeprom_read_block(solde,&ee_solde,2);
	
	sendbytet0(solde[1]);//little endian
	sendbytet0(solde[0]);//si BE on inverse les 2, le 1 passe avant le 0
	sw1 = 0x90;
}
void init_solde()
{
	if(p3!=2)   //2octets
	{
	sw1=0x6c;
	sw2=2;  //taille 2 octets
	return;
	}
	sendbytet0(ins); //acquittement
	uint16_t solde ;
	solde = 0	; 
	eeprom_write_block(&solde, &ee_solde, 2);
	sw1=0x90;
	sw2=00;
}
	


// Programme principal
//--------------------
int main(void)
{
  	// initialisation des ports
	ACSR=0x80;
	DDRB=0xff;
	DDRC=0xff;
	DDRD=0;
	PORTB=0xff;
	PORTC=0xff;
	PORTD=0xff;
	ASSR=1<<EXCLK;
	TCCR2A=0;
	ASSR|=1<<AS2;


	// ATR
  	atr(11,"Hello scard");

	taille=0;
	sw2=0;		// pour éviter de le répéter dans toutes les commandes
  	// boucle de traitement des commandes
  	for(;;)
  	{
    		// lecture de l'entête
    		cla=recbytet0();
    		ins=recbytet0();
    		p1=recbytet0();
	    	p2=recbytet0();
    		p3=recbytet0();
	    	sw2=0;
		switch (cla)
		{
	  	case 0x80:
		    	switch(ins)
			{
			case 0:
				version(4,"1.00");
				break;
		  	case 1:
	        		intro_data();
	        		break;
            		case 2:
				sortir_data();
				break;
			case 3:
				crediter ();
				break;
			case 4:
				debiter ();
				break;
			case 5:
				sortir_solde();
				break;
			case 6
			

            default:
		    		sw1=0x6d; // code erreur ins inconnu
			}
		break;

      	default:
        		sw1=0x6e; // code erreur classe inconnue
		}
		sendbytet0(sw1); // envoi du status word
		sendbytet0(sw2);
  	}
  	return 0;
}


