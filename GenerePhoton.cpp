//*******************************************************************
//*																	*
//* Programme: GenerePhoton.cpp	       							    *
//*																	*
//*******************************************************************
//*																	*
//* Description:													*
//*		Génération de photons.                     					*
//*																	*
//* nom1 :  Vaucher Quentin                                         *
//* matricule1: 17 133 004                                          *
//*																	*
//* nom2 : Guerne Jonathan                                          *
//* matricule2: 17 133 022                                          *
//*******************************************************************


#include "GenerePhoton.h"
#include "inter.h"
#include "util.h"
#include "utilalg.h"
#include "aff.h"
#include "segment.h"
#include "ensemble.h"
#include "geo.h"
#include "mat.h"
#include <math.h>
#include "Point.h"
#include "time.h"
#include "main.h"

#include "spotlight.h"
#include "ponctuelle.h"
#include "settingTracePhoton.h"




booleen
GenerePhotons(const Camera& camera, Objet* scene)
{


	const Lumiere* lum = NULL;

	PhotonMap* CaustiqueMap = pFenAff3D->PhotonTracing()->PhotonMapCaustique();

	int nbLum = camera.NbLumiere(); // pour avoir le nombre de lumiere
	lum = camera.GetLumiere(0);  // la première lumiere : n'oubliez pas les autres ...


	printf("\nGeneration des photons...\n");

	clock_t  clk = clock();

	//	... à compléter

	reel energieTotalSLum = 0;

	for (int i = 0; i < nbLum; i++) {
		energieTotalSLum = energieTotalSLum + 
			camera.GetLumiere(i)->EnergiePhoton().rouge() + 
			camera.GetLumiere(i)->EnergiePhoton().vert() + 
			camera.GetLumiere(i)->EnergiePhoton().bleu();
	}

	for (int i = 0; i < nbLum; i++) {

		point pt_lumiere = camera.Position(i);
		cout << "R" <<  camera.GetLumiere(i)->EnergiePhoton().rouge() << "G" << camera.GetLumiere(i)->EnergiePhoton().vert() << "B" << camera.GetLumiere(i)->EnergiePhoton().bleu() <<  endl;

		lum = camera.GetLumiere(i);
		int nbPhotonsLumI = (lum->EnergiePhoton().rouge() + 
			lum->EnergiePhoton().vert() + 
			lum->EnergiePhoton().bleu()) / 
			energieTotalSLum * NB_PHOTON_CAUSTIQUE;

		cout << "nbPhotonLumI" << nbPhotonsLumI << endl;

		for (int j = 0; j < nbPhotonsLumI; j++) {

			vecteur direction = camera.GetLumiere(i)->RayonAleatoire();

			bool is_reflected;
			int nb_reflexion = 0;

			reel *k = new reel();
			vecteur *vn = new vecteur();
			Couleurs *couleurs = new Couleurs();

			point pt_inter = pt_lumiere;
			
			Couleur puissance_photon = lum->EnergiePhoton() / nbPhotonsLumI;

			do {
				is_reflected = false;

				if (Objet_Inter(*scene, pt_inter, direction, k, vn, couleurs) && couleurs->reflechi() != Couleur(0, 0, 0)) {
					pt_inter = pt_inter + *k * direction;
					direction = Reflechi(direction, *vn);

					puissance_photon = puissance_photon * couleurs->reflechi();

					is_reflected = true;
					nb_reflexion++;
				}
			} while (is_reflected);

			if (nb_reflexion > 0) {
				direction.normalise();
				CaustiqueMap->Store(puissance_photon, pt_inter, direction);
			}

		}

		CaustiqueMap->Balance();
	}


	printf("\nFin du trace de photon\n");

	printf("Balancement du photon map caustique...\n");
	CaustiqueMap->Balance();
	printf("Fin du balancement du photon map caustique\n\n");

	clk = clock() - clk;
	clk /= CLOCKS_PER_SEC;

	Affiche(*CaustiqueMap, &scene, camera);


	printf("Temps pour generer les photons : %d:%02d\n\n", clk / 60, clk % 60);

	return VRAI;
}



void Affiche(const PhotonMap& table, Objet **scene, const Camera& camera)
{
	Attributs* a = new Attributs;

	Couleur cd(1, 1, 1);

	a->diffus(cd);
	a->ambiant(cd);

	Ensemble* ens = new Ensemble;
	const Lumiere* lum = NULL;

	for (int i = 1; i <= table.NbPhotons(); i += 100)
	{
		point p1;
		p1 = table[i].position();


		vecteur vec;
		vec = table[i].PhotonDir();

		vec = -vec;

		point p2 = p1 + (vec);

		Segment* seg = new Segment(p1, p2);
		seg->attributs(a);
		ens->ajoute(seg);
	}

	// affiche la direction du spot light
	for (int j = 0; j < camera.NbLumiere(); j++)
	{
		lum = camera.GetLumiere(j);

		if (lum->Type() == unSpotlight)
		{
			Spotlight * spot = (Spotlight*)lum;
			// direction du spot light
			point p2 = spot->Position() + (spot->GetDirection() * 2);
			Segment* seg = new Segment(spot->Position(), p2);

			seg->attributs(a);
			ens->ajoute(seg);
		}
	}

	ens->attributs(a);

	((Ensemble*)(*scene))->ajoute(ens);
}