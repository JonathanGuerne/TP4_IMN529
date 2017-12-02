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

	for (int i = 0; i < camera.NbLumiere(); i++) {

		vecteur l = (camera.Position(i) - pt_inter).unitaire();
		vecteur h = ((l + o) * (1 / (l + o).norme())).unitaire();
		reel cosAlpha = (h * (*vn) < 0) ? 0 : h * (*vn);
		reel cosTeta = (*vn) * l;

		if (cosTeta < 0) {
			cosAlpha = 0;
			cosTeta = 0;
		}



		Couleur id = camera.GetLumiere(i)->Intensite() * c->diffus() *cosTeta;
		//Couleur is = camera.Diffuse(i) * c->speculaire() * pow(cosAlpha, 90);
		//Couleur ia = (camera.Ambiante(i)*c->ambiant());

		//ombre 
		vecteur ombreDir = vecteur(camera.Position(i), pt_inter);

		reel* ombreK = new reel();
		vecteur* ombreVn = new vecteur();
		Couleurs* ombreC = new Couleurs();

		//if (!Objet_Inter(*scene, pt_inter, ombreDir, ombreK, ombreVn, ombreC)) {
			intensite = intensite + id;
		//}
		//else {
		//	intensite = intensite;
		//}
	}

	if (c->ombre() != Couleur(0.0, 0.0, 0.0)) {
		vecteur ro = o; //todo trouver le vecteur réfléchis de o
		Couleur im = calcul_intens_rayon(scene, pt_inter, o, camera);
		intensite = intensite + im*c->reflechi();
	}

	return intensite;
}

Couleur calcul_intens_rayon(Objet* scene, point origine, vecteur direction, const Camera& camera)
{
	reel* k = new reel();
	vecteur* vn = new vecteur();
	Couleurs* c = new Couleurs();

	Couleur intensite;

	PhotonMap* CaustiqueMap = pFenAff3D->PhotonTracing()->PhotonMapCaustique();

	if (Objet_Inter(*scene, origine, direction, k, vn, c)) {

		reel rayon = 0.3;
		int nbPhotons = 200, found;

		reel *dist2 = NULL;

		const Photon **ph = NULL;

		point pt_inter = origine + direction*(*k);

		intensite = calcul_intensite_point_inter(scene, camera, direction, pt_inter, vn, c);

		CaustiqueMap->Locate(pt_inter, rayon, nbPhotons, found, &dist2, &ph);

		Couleur sumEnergiePhoton = Couleur(0, 0, 0);

		for (int i = 1; i < found; i++) {
			sumEnergiePhoton = sumEnergiePhoton + (ph[i]->energie() * c->diffus());
		}

		intensite = intensite + (sumEnergiePhoton / PI * pow(rayon, 2));
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


	point origineParTransfo = transfInv.transforme(point(0.0, 0.0, 0.0));

	point origine = camera.PO();
	std::cout << "origine x: " << origine.x() << " y: " << origine.y() << " z: " << origine.z() << std::endl;
	std::cout << "or tran x: " << origineParTransfo.x() << " y: " << origineParTransfo.y() << " z: " << origineParTransfo.z() << std::endl;

	reel dx = 2.0 / nb_pixel_x;
	reel dy = 2.0 / nb_pixel_y;

	//origine = transfInv.inverse(camera.PO());
	//std::cout << "x: " << origine.x() << " y: " << origine.y() << " z: " << origine.z() << std::endl;
	//
	//origine = transfInv.transforme(transfInv.inverse(camera.PO()));
	//std::cout << "x: " << origine.x() << " y: " << origine.y() << " z: " << origine.z() << std::endl;

	// ...

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

			point pointMilieu = transfInv.transforme(point(x, y, 1.0));

			//if (no_x % 10 == 0 && no_y % 10 == 0) {
			//	std::cout << "x " << pointMilieu.x() << ";y " << pointMilieu.y() << endl;
			//}

			vecteur directionRayon = vecteur(origine, pointMilieu); // l'ordre des points est juste 

			Intensite = calcul_intens_rayon(scene, origine, directionRayon, camera);

			// ...
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

