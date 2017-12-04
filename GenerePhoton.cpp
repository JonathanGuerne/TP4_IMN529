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


reel puissanceSLum(const Couleur& couleur) {
	return couleur.rouge() + couleur.vert() + couleur.bleu();
}

booleen
GenerePhotons(const Camera& camera, Objet* scene)
{
	const Lumiere* lum = NULL;

	PhotonMap* CaustiqueMap = pFenAff3D->PhotonTracing()->PhotonMapCaustique();
	
	int nbLum = camera.NbLumiere(); // pour avoir le nombre de lumiere

	printf("\nGeneration des photons...\n");

	clock_t  clk = clock();

	reel energieTotalSLum = 0;
	for (int i = 0; i < nbLum; i++) {
		energieTotalSLum += puissanceSLum(camera.GetLumiere(i)->EnergiePhoton());
	}

	for (int i = 0; i < nbLum; i++) {	
		
		lum = camera.GetLumiere(i);
		reel phtonsParEnergieSLum = NB_PHOTON_CAUSTIQUE / energieTotalSLum;
		reel nbPhotonsLumI = puissanceSLum(lum->EnergiePhoton()) * phtonsParEnergieSLum;
		Couleur puissance_photon = lum->EnergiePhoton() / nbPhotonsLumI;

		for (int j = 0; j < nbPhotonsLumI; j++) {
			point origine = lum->Position();
			vecteur direction = lum->RayonAleatoire();

			bool is_reflected = true;
			int nb_reflexion = 0;

			
			while (is_reflected) {

				reel *k = new reel();
				vecteur *vn = new vecteur();
				Couleurs *couleurs = new Couleurs();

				is_reflected = false;

				if (Objet_Inter(*scene, origine, direction, k, vn, couleurs) ) {
					origine = origine + direction * (*k);

					if (couleurs->reflechi() != Couleur(0.0, 0.0, 0.0)) {
						direction = Reflechi(direction, *vn);
						puissance_photon = puissance_photon * couleurs->reflechi();

						is_reflected = true;
						nb_reflexion++;
					}
				}
			}

			if (nb_reflexion > 0) {
				direction.normalise();
				CaustiqueMap->Store(puissance_photon, origine, direction);
			}

		}
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