/*
This work is licensed under the Creative Commons Attribution
- Pas d’Utilisation Commerciale - Partage dans les Mêmes Conditions
3.0 France License. To view a copy of this license, visit
http://creativecommons.org/licenses/by-nc-sa/3.0/fr/.
*/

#ifndef drawbot
#define drawbot

#include "params.h"
#include <Arduino.h>
#include <math.h>
#include <SD.h>
#include <Servo.h> 

class Drawbot {
    public:

    /****************
    ** construceurs **
    ****************/

    // dimentions de la surface (en mm)
    Drawbot(int distanceBetweenMotors, int sheetWidth, int sheetHeight, int sheetPositionX, int sheetPositionY);

    /**********************
    ** getters & setters **
    **********************/

    // renvoie la position courante en X du crayon
    float getPositionX();

    // renvoie la position courante en Y du crayon
    float getPositionY();

    // initialise la position du curseur, à n'utiliser qu'au début.
    // Par défaut: au centre de la surface
    void setPosition(float aX, float aY);

    // initialise la vitesse du crayon
    void setSpeed(float vitesse);
    
    /*****************
    ** alimentation **
    *****************/
    
    // obligatoire, à placer en début de programme :
    // réalise les procédures d'initialisation nécessaires au fonctionnement du robot.
    void begin();
    
    /*****************
    ** deplacements **
    *****************/

    // Quasiment toutes ces fonctions sont doublée,
    // de manière à effectuer le traçé: 
    // - en position absolue (ABS)
    // - en position relative (REL)

    // deplacements (sans écrire)

    // place le crayon au point [x, y]
    void move(float x, float y);
    void _move(float x, float y);

    // place le crayon au centre de la surface.
    
    // TODO : remplacer par setPosition(CENTER);
    void centrer();

    void end();
    // lignes droite:
    
    // trace une ligne droite, du point courant au point [x, y]
    void line(float x, float y);
    void _line(float x, float y);
    
    // lignes horizontales/verticales

    // trace une ligne horizontale sur toute la largeur de la surface, à la coordonée y.
    void horizontal(float y);
    void _horizontal(float y);

    // trace une ligne verticale sur toute la hauteur de la surface,
    // à la coordonée x.
    void vertical(float x);
    void _vertical(float x);

    // courbes de Bezier cubiques
    
    // trace une courbe de Bézier cubique
    // dont les points de controle P1 et P2 sont respectivement
    // en [x1, y1] et [x2, y2],
    // et dont le point de destination est en [x, y].
    void cubicCurve(float x, float y, float x1, float y1, float x2, float y2);
    void _cubicCurve(float x, float y, float x1, float y1, float x2, float y2);

    // idem, le point de controle P1 est l'image du point P2 de la dernière courbe de Bézier
    // (cubique ou quadratique) par symétrie avec le point courant.
    // Si la dernière figure n'était pas une courbe de Bézier, il correspond au point courant.
    void cubicCurve(float x, float y, float x2, float y2);
    void _cubicCurve(float x, float y, float x2, float y2);

    //courbes de Bezier quadratiques

    // trace une courbe de Bézier quadratique dont le point de controle P1 est en [x1, y1]
    // et dont le point de destination est en [x, y].
    void quadraticCurve(float x, float y, float x1, float y1);
    void _quadraticCurve(float x, float y, float x1, float y1);

    // idem, le point de controle est l'image du point de controle de la dernière courbe de Bézier
    // (cubique ou quadratique) par symétrie avec le point courant.
    // Si la dernière figure n'était pas une courbe de Bézier, il correspond au point courant.
    void quadraticCurve(float x, float y);
    void _quadraticCurve(float x, float y);
    
    // arcs et cercles
    
    // trace un arc de cercle
    void arc(float a1, float a2, float a3, float a4, float a5, float a6, float a7); // non fini
    void _arc(float a1, float a2, float a3, float a4, float a5, float a6, float a7);
    
    // trace une ellipse dont le centre est les point courant,
    // le rayon horizontal est rx et le rayon vertical est ry.
    void ellipse(float rx, float ry);

    // trace un cercle dont le centre est les point courant et de rayon r.
    void circle(float r);
    
    void svg(char* fileName);
    
    private:

    // ligne(float x, float y, bool write) n'initialise pas les variables pour les courbes Bezier, à la différence de line(float x, float y)
    void line(float x, float y, bool writing);
    
    // déplacement d'un pas moteur
    void leftStep();
    void rightStep();
    
    // fini le traçé courant:
    // Revient au point cible du dernier déplacement.
    // Ceci permet de faire des formes "finies"
    void endCurve();

    // renvoie la longueur courante du câble gauche
    long getLeftLength();
    
    // renvoie la longueur courante du câble droit
    long getRightLength();
    
    // initialise le ratio pour le calcul de distance, à n'utiliser qu'au début.
    // Par défaut: 100
    
    // TODO : Remplacer par SetScaleX et setScaleY
    void initRatio();

    void setScale(int width, int height);
    
    // alimente ou désalimente le moteur.
    // Attention, selon le poids du crayon, la désalimentation peut entrainer la chute du robot.
    void power(bool alimenter);

    void error(char* errNumber);
    
    /*********************
    ** lecture carte SD **
    *********************/
    
    // initialisation de la carte SD
    int sdInit();

    // recupere une valeur du fichier XML
    // EX: width --> (21cm) OU style --> (fill:none;stroke:#000000)
    void getAttribute(const char *attribute, char *value);

    // recupere une valeur du fichier XML qui est un nombre et le converti en float,
    // en prenant en compte les unités (cm, mm, ...)
    // EX: width --> 210 OU height --> 320
    float getNumericAttribute(const char *attribute);

    // teste si le car est un chiffre ou non
    bool isNumber(char car);

    // trouve un mot dans le fichier
    // renvoie faux si non trouvé (le curseur sera à la fin du mot)
    boolean sdFind(const char *mot);

    // récupère nbParams paramètres, les convertis en float et les stoquent dans le tableau tNb
    void getParameters(float *tNb, int nbParams);

    // dessine les traçés en lisant le contenu du fichier .svg
    void draw();

    long positionToLeftLength(float x, float y);
    long positionToRightLength(float x, float y);

    // si ecrire est vrai, le stylo est contre la paroi, sinon il est éloigné
    void write(bool write);

    /************
    * Attributs *
    *************/

    Servo mServo;

    unsigned int mLeftLength;
    unsigned int mRightLength;

    int mDistanceBetweenMotors;
    int mSheetPositionX;
    int mSheetPositionY;
    
    // le fichier XML à lire
    File mFile;
    
    float mPositionX;
    float mPositionY; // position sur le plan
    
    float mFictivePosX; // position voulue (fictive) avant qu'elle ne soit modifiée par les limites
    float mFictivePosY;    

    float mSheetWidth;
    float mSheetHeight; // taille du plan (en mm)
    
    float mRatio; // ratio entre le nb de pas et les mm
    
    float mDelay;
    
    bool mWriting;
    
    float mStartCurveX;    
    float mStartCurveY;
    
    float mCubicCurveX;
    float mCubicCurveY;
    float mQuadraticCurveX;
    float mQuadraticCurveY;    
};

#endif
