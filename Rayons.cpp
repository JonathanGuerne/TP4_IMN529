/******************************************************************/
/*                                                                */
/* Vaucher de la Croix 17 133 004                                 */
/*                                                                */
/*                                                                */
/*                                                                */
/*                                                                */
/*                                                                */
/*                                                                */
/*                                                                */
/*                                                                */
/*                                                                */
/******************************************************************/






#include "rayons.h"
#include "couleurs.h"
#include "attr.h"
#include "ensemble.h"
#include "point.h"
#include "transfo.h"
#include "inter.h"
#include "vision.h"
#include "util.h"
#include "utilalg.h"
#include "Photon.h"
#include <stdio.h>
#include <math.h>
#include "Main.h"


Couleur calcul_intens_rayon(Objet* scene, point origine, vecteur direction, const Camera& camera);

void Enregistre_pixel(int no_x, int no_y, Couleur intens, Fichier f)
// enregistre la couleur du pixel dans le fichier f
// Chacune des composantes de la couleur doit etre de 0 a 1,
// sinon elle sera ramene a une borne.
{

	reel r, v, b;
	char c;

	intens = intens*255.0;
	r = intens.rouge();
	if (r < 0) r = 0;
	if (r > 255)r = 255;
	c = (unsigned char)r;
	f.Wcarac(c);

	v = intens.vert();
	if (v < 0) v = 0;
	if (v > 255)
		v = 255;
	c = (unsigned char)v;
	f.Wcarac(c);

	b = intens.bleu();
	if (b < 0) b = 0;
	if (b > 255) b = 255;
	c = (unsigned char)b;
	f.Wcarac(c);

}

Couleur calcul_intensite_point_inter(Objet* scene, const Camera& camera, vecteur direction, point pt_inter, vecteur* vn, Couleurs* c)
{
	Couleur intensite(0.0, 0.0, 0.0);

	vecteur o = -direction.unitaire();

	(*vn).unitaire();

	// Vérification du sens du vecteur normal
	if ((*vn) * o < 0) {
		*vn = -(*vn);
	}

	PhotonMap* CaustiqueMap = pFenAff3D->PhotonTracing()->PhotonMapCaustique();

	reel rayon_max = 0.3;

	int nbPhotons = 200, found;

	reel *dist2 = NULL;

	const Photon **ph = NULL;

	for (int i = 0; i < camera.NbLumiere(); i++) {

		// Calcul de cos(Theta) et cos(Alpha)
		vecteur l = (camera.Position(i) - pt_inter).unitaire();
		vecteur h = ((l + o) * (1 / (l + o).norme())).unitaire();
		reel cosAlpha = (h * (*vn) < 0) ? 0 : h * (*vn);
		reel cosTheta = (*vn) * l;

		if (cosTheta < 0) {
			cosAlpha = 0;
			cosTheta = 0;
		}

		// Coefficients diffus, spéculaire et ambiant
		Couleur id = camera.GetLumiere(i)->Intensite() * c->diffus() * cosTheta;
		Couleur is = Couleur(0, 0, 0);// camera.GetLumiere(i)->Intensite() * c->speculaire() * pow(cosAlpha, 90);
		Couleur ia = Couleur(0, 0, 0);// (camera.GetLumiere(i)->IntensiteAmbiante() *c->ambiant());


		//ombre 
		vecteur ombreDir = vecteur(pt_inter, camera.Position(i)).unitaire();

		reel* ombreK = new reel();
		vecteur* ombreVn = new vecteur();
		Couleurs* ombreC = new Couleurs();

		if (!camera.GetLumiere(i)->Eclaire(pt_inter)) {
			id = 0;
			is = 0;
		}

		intensite = intensite + id + is + ia;

	}

	CaustiqueMap->Locate(pt_inter, rayon_max, nbPhotons, found, &dist2, &ph);

	Couleur sumEnergiePhoton = Couleur(0, 0, 0);
	Couleur caustiqueIntensite = Couleur(0, 0, 0);

	reel rayon2 = 0;

	if (found > 0) {
		for (int i = 1; i <= found; i++) {

			ph[i]->PhotonDir().normalise();
			vn->normalise();

			//cout << "acos: " << acos(((*vn) * ph[i]->PhotonDir()) / (vn->norme() * ph[i]->PhotonDir().norme())) << endl;

			//if (abs(acos((*vn * ph[i]->PhotonDir()) / (vn->norme() * ph[i]->PhotonDir().norme()))) < (PI / 2)) {
				//cout << "TOTO" << endl;
				sumEnergiePhoton = sumEnergiePhoton + ph[i]->energie() * c->diffus();
			//}

			if (*dist2 > rayon2) {
				rayon2 = *dist2;
			}
		}

		reel pi_rayon = PI * rayon2;

		caustiqueIntensite = (sumEnergiePhoton / pi_rayon);
	}

	delete[] dist2;
	delete[] ph;

	intensite = intensite + caustiqueIntensite;

	if (c->reflechi() != Couleur(0.0, 0.0, 0.0)) {
		vecteur ro = Reflechi(-o, (*vn));  // Vecteur du rayon réfléchi

		Couleur im = calcul_intens_rayon(scene, pt_inter, ro, camera);
		intensite = intensite + im * c->reflechi();
	}

	return intensite;
}

Couleur calcul_intens_rayon(Objet* scene, point origine, vecteur direction, const Camera& camera)
{
	reel* k = new reel();
	vecteur* vn = new vecteur();
	Couleurs* c = new Couleurs();

	Couleur intensite(0.0, 0.0, 0.0);

	if (Objet_Inter(*scene, origine, direction, k, vn, c)) {
		point pt_inter = origine + direction*(*k);
		intensite = calcul_intensite_point_inter(scene, camera, direction, pt_inter, vn, c);
	}

	else {
		intensite = Couleur(0.0, 0.0, 0.0); // black (background)
	}

	return intensite;
}

booleen TraceRayons(const Camera& camera, Objet *scene, const entier& res, char fichier[])
// Genere un rayon pour chacun des pixel et calcul l'intensite de chacun
{
	Couleur Intensite(0.0, 0.0, 0.0);

	entier nb_pixel_x = res;
	entier nb_pixel_y = res;

	Transformation transfInv = Vision_Normee(camera).inverse(); // transformation de vision inverse

	point origine = camera.PO();

	reel dx = 2.0 / nb_pixel_x;
	reel dy = 2.0 / nb_pixel_y;

	// Ouverture du fichier pour y enregistrer le trace de rayon
	Fichier f;
	if (!f.Open(fichier, "wb")) return FAUX;

	f.Wchaine("P6\r");
	f.Wentier(res); f.Wcarac(' '); f.Wentier(res);	f.Wchaine("\r");
	f.Wentier(255);	f.Wchaine("\r");

	printf("\nDebut du trace de rayons\n");
	printf("%4d source(s) lumineuse(s)\n", camera.NbLumiere());

	// ...

	for (int no_y = 1; no_y <= nb_pixel_y; no_y++)
	{
		for (int no_x = 1; no_x <= nb_pixel_x; no_x++)
		{
			reel x = 1.0 - dx / 2.0 - (no_x - 1) * dx;
			reel y = 1.0 - dy / 2.0 - (no_y - 1) * dy;

			point pointMilieu = transfInv.transforme(point(x, y, 1.0)); // Coordonnées du milieu du pixel en CU

			vecteur directionRayon = vecteur(origine, pointMilieu);

			Intensite = calcul_intens_rayon(scene, origine, directionRayon, camera);

			Enregistre_pixel(no_x, no_y, Intensite, f);
		}

		//Imprime le # de ligne rendu
		if (no_y % 15 == 0) printf("\n");
		printf("%3d ", no_y);
	}
	printf("\n\nFin du trace de rayons.\n");


	f.Close();
	return VRAI;
}

