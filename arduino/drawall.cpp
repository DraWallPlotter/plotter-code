/*
 * This file is part of DraWall.
 * DraWall is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * DraWall is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU
 * General Public License along with DraWall. If not, see <http://www.gnu.org/licenses/>.
 * © 2012–2014 Nathanaël Jourdane
 * © 2014 Victor Adam
 */

/**
 * Main library file.
 */

#include <drawall.h>

// TODO config file name should be constant and not user settable,
// since this file will be send through the serial port in a future release.

// TODO remove all public methods, now they are useless and it will save space.

void Drawall::begin() {
	// Affectation des pins des moteurs
	pinMode(PIN_ENABLE_MOTORS, OUTPUT);
	pinMode(PIN_LEFT_MOTOR_STEP, OUTPUT);
	pinMode(PIN_LEFT_MOTOR_DIR, OUTPUT);
	pinMode(PIN_RIGHT_MOTOR_STEP, OUTPUT);
	pinMode(PIN_RIGHT_MOTOR_DIR, OUTPUT);

	pinMode(PIN_STEP_MODE_0, OUTPUT);
	pinMode(PIN_STEP_MODE_1, OUTPUT);
	pinMode(PIN_STEP_MODE_2, OUTPUT);

	// Pin carte SD
	pinMode(PIN_SD_CS, OUTPUT);

#ifdef BUTTONS
	/* Étrange, ces lignes font foirer la communication série...
	 // Gestion de l'interruption à l'appui sur le BP pause.
	 SREG |= 128;   // Interruptions globales ok
	 EIMSK = B01;   // Interruption INT0 déclenchée à l'appui sur le BP pause
	 // INT0 (pin2) = B01 ou INT1 (pin3) = B10 sur Atmega328.
	 EICRA = B0011; // Interruption déclenchée sur le front montant pour INT0
	 // état bas = B00, changement d'état = B01
	 // front descendant = B10, front montant = B11
	 */

	pinMode(PIN_LEFT_CAPTOR, INPUT);
	pinMode(PIN_RIGHT_CAPTOR, INPUT);
	pinMode(PIN_REMOTE, INPUT);

	// Activation des pullup internes pour les boutons
	digitalWrite(PIN_PAUSE, HIGH);
	digitalWrite(PIN_LEFT_CAPTOR, HIGH);
	digitalWrite(PIN_RIGHT_CAPTOR, HIGH);
#endif

	// Affectation des pin de l'écran
#ifdef SCREEN
	pinMode(PIN_SCREEN_SCE, OUTPUT);
	pinMode(PIN_SCREEN_RST, OUTPUT);
	pinMode(PIN_SCREEN_DC, OUTPUT);
	pinMode(PIN_SCREEN_SDIN, OUTPUT);
	pinMode(PIN_SCREEN_SCLK, OUTPUT);
#endif

#ifdef SERIAL
	// Début de la communication série.
	Serial.begin(SERIAL_BAUDS);
#endif

	if (!SD.begin(PIN_SD_CS)) {
		error(ERR_CARD_NOT_FOUND);
	}

	// Chargement des paramètres à partir du fichier de configuration
	loadParameters();

	mServo.attach(PIN_SERVO);
	mServo.write(mpServoMoovingAngle);

	mIsWriting = true; // Pour que write() fonctionne la 1ere fois

	// Vérification de la distance entre les 2 moteurs.
	if (mpSpan < mpSheetWidth + mpSheetPositionX) {
		error(ERR_TOO_SHORT_SPAN);
	}

	setStepMode(mpStepMode); // call initStepLength()
	setPosition(mpInitPosition);

	// Calcul de la longueur des courroies au début
	mLeftLength = positionToLeftLength(mPositionX, mPositionY);
	mRightLength = positionToRightLength(mPositionX, mPositionY);

	setSpeed(mpDefaultSpeed);

#ifdef SERIAL
	Serial.println("READY");
	delay(100);
#endif

#ifdef SERIAL
	// Send init data to computer
	Serial.write(DRAW_START_INSTRUCTIONS);
	Serial.println(mpSpan);
	Serial.println(mpSheetPositionX);
	Serial.println(mpSheetPositionY);
	Serial.println(mpSheetWidth);
	Serial.println(mpSheetHeight);
	Serial.println(mLeftLength);
	Serial.println(mRightLength);
	Serial.println(mStepLength * 1000);
	Serial.write(DRAW_END_INSTRUCTIONS);
#endif

#ifdef BUTTONS
	// Pause until button pushed.
	// Serial.println("_Press button to begin...");
	// while(digitalRead(PIN_BP) == LOW) {}
#endif

	power(true);

	mPositionZ = 10;

	// TODO: message: waiting for xx seconds
	delay(mpInitialDelay);
	//mDrawingHeight = mpSheetHeight; // Pour ne pas soustraire la hauteur du dessin
}

// Interrupt routine
/*#ifdef BUTTONS
 * ISR(INT0_vect)
 * {
 * EIMSK = 0;                   // Bloquage de INT0
 *
 * while (PIND & 4) {
 * };                           // Attente que le pin soit à '1'
 * while (!(PIND & 4)) {
 * };                           // Attente que le pin soit à '0'
 * // (Pour un front descendant, inverser les 2 lignes).
 *
 * EIMSK = 1;                   // Réautorisation de INT0
 * EIFR = 1;                    // Flag de INT0 remis à '0'
 * };
 * #endif */

void Drawall::setStepMode(byte stepMode) {
	digitalWrite(PIN_STEP_MODE_0, (stepMode & B1) > 0 ? HIGH : LOW);
	digitalWrite(PIN_STEP_MODE_1, (stepMode & B10) > 0 ? HIGH : LOW);
	digitalWrite(PIN_STEP_MODE_2, (stepMode & B100) > 0 ? HIGH : LOW);
	initStepLength();
}

void Drawall::waitUntil(char expected) {
	char received = 0;

	while (received != expected) {
		while (Serial.available() < 1)
			; // Attend un octet
		received = Serial.read();       // Lit l'octet
	}
}

void Drawall::setPosition(float positionX, float positionY) {
	mPositionX = positionX;
	mPositionY = positionY;
}

void Drawall::setPosition(CardinalPoint position) {
	setPosition(positionToX(position), positionToY(position));
}

int Drawall::positionToX(CardinalPoint position) {
	switch (position) {
	case UPPER_CENTER:
	case CENTER:
	case LOWER_CENTER:
		return mpSheetWidth / 2;

	case UPPER_RIGHT:
	case RIGHT_CENTER:
	case LOWER_RIGHT:
		return mpSheetWidth;

	default:
		return 0;
	}
}

int Drawall::positionToY(CardinalPoint position) {
	switch (position) {
	case UPPER_LEFT:
	case UPPER_CENTER:
	case UPPER_RIGHT:
		return mpSheetHeight;

	case LEFT_CENTER:
	case CENTER:
	case RIGHT_CENTER:
		return mpSheetHeight / 2;

	default:
		return 0;
	}
}

void Drawall::initStepLength() {
	// mpSteps*2 car sur le driver c'est seulement le front montant qui contôle les pas du moteur
	mStepLength = (PI * mpDiameter / 1000) / (mpSteps * 2 * pow(2, mpStepMode));
}

void Drawall::setSpeed(unsigned int speed) {
	mDelay = 1000000 * mStepLength / float(speed);
}

long Drawall::positionToLeftLength(float positionX, float positionY) {
	return sqrt(
			pow(((float) mpSheetPositionX + positionX) / mStepLength, 2)
					+ pow(
							((float) mpSheetPositionY + mpSheetHeight
									- positionY) / mStepLength, 2));
}

long Drawall::positionToRightLength(float positionX, float positionY) {
	return sqrt(
			pow(
					((float) mpSpan - (float) mpSheetPositionX - positionX)
							/ mStepLength, 2)
					+ pow(
							((float) mpSheetPositionY + mpSheetHeight
									- positionY) / mStepLength, 2));
}

void Drawall::power(bool shouldPower) {
	if (shouldPower) {
		digitalWrite(PIN_ENABLE_MOTORS, LOW);
#ifdef SERIAL
		Serial.write(DRAW_ENABLE_MOTORS);	// Processing: a = alimenter
#endif
	} else {
		digitalWrite(PIN_ENABLE_MOTORS, HIGH);
#ifdef SERIAL
		Serial.write(DRAW_DISABLE_MOTORS);	// Processing: b = désalimenter
#endif
		writingPen(false);
	}
}

// À supprimer plus tard, mais garder pour l'instant
void Drawall::writingPen(bool shouldWrite) {
	if (shouldWrite && !mIsWriting) {
		// Si on veut écrire mais que le stylo n'écrit pas
		delay(mpPreServoDelay);
		mServo.write(mpServoWritingAngle);
		delay(mpPostServoDelay);

#ifdef SERIAL
		Serial.write(DRAW_WRITING);	// Processing: w = écrire
#endif
		mIsWriting = true;
	} else if (!shouldWrite && mIsWriting) {
		// Si on ne veut pas écrire mais que le stylo écrit
		delay(mpPreServoDelay);
		mServo.write(mpServoMoovingAngle);
		delay(mpPostServoDelay);

#ifdef SERIAL
		Serial.write(DRAW_MOVING);	// Processing: x = ne pas écrire
#endif
		mIsWriting = false;
	}
}

void Drawall::leftStep(bool shouldPull) {
	if (shouldPull) {
		mLeftLength--;
#ifdef SERIAL
		Serial.write(DRAW_PULL_LEFT);
#endif
	} else {
		mLeftLength++;
#ifdef SERIAL
		Serial.write(DRAW_RELEASE_LEFT);
#endif
	}

	digitalWrite(mpReverseMotors ? PIN_RIGHT_MOTOR_STEP : PIN_LEFT_MOTOR_STEP,
			mLeftLength % 2);
}

void Drawall::rightStep(bool shouldPull) {
	if (shouldPull) {
		mRightLength--;
#ifdef SERIAL
		Serial.write(DRAW_PULL_RIGHT);
#endif
	} else {
		mRightLength++;
#ifdef SERIAL
		Serial.write(DRAW_RELEASE_RIGHT);
#endif
	}

	digitalWrite(mpReverseMotors ? PIN_LEFT_MOTOR_STEP : PIN_RIGHT_MOTOR_STEP,
			mRightLength % 2);
}

// TODO: Gérer l'axe Z
void Drawall::line(float x, float y) {
	writingPen(true);
	int longmax = 5;

	float longX = abs(x - mPositionX);
	float longY = abs(y - mPositionY);

	float miniX;
	float miniY;
	int boucle;

	if (longX > longmax || longY > longmax) {
		boucle = ceil((longX > longY ? longX : longY) / longmax);
		miniX = (x - mPositionX) / boucle;
		miniY = (y - mPositionY) / boucle;

		for (int i = 0; i < boucle; i++) {
			segment(mPositionX + miniX, mPositionY + miniY, true);
		}
	}
	segment(x, y, true);
}

// TODO: à supprimer plus tard, quand on utilisera l'axe Z pour gérer les mouvements.
void Drawall::move(float x, float y) {
	writingPen(false);
	segment(x, y, false);
}

void Drawall::move(CardinalPoint position) {
	move(positionToX(position), positionToY(position));
}

// TODO: gérer l'axe Z
void Drawall::processSDLine() {
#define PARAM_MAX_LENGTH 5
#define FUNC_MAX_LENGTH 3

	byte i, j;
	char functionName[FUNC_MAX_LENGTH + 1];
	char car;
	char parameter[PARAM_MAX_LENGTH + 1];
	float parameters[2]; // TODO use integers

	// Get function name
	car = mFile.read();
	for (i = 0; car != ' ' && car != '\n' && i < FUNC_MAX_LENGTH + 1; i++) {
		functionName[i] = car;
		car = mFile.read();
	}
	functionName[i] = '\0';

	// Get parameters
	// The first char has been already read.
	for (i = 0; car != '\n'; i++) {
		car = mFile.read(); // first param char (X, Y or Z)
		for (j = 0; car != ' ' && car != '\n' && j < PARAM_MAX_LENGTH + 1;
				j++) {
			if (j == 0) {
				car = mFile.read(); // pass first param char
			}
			parameter[j] = car;
			car = mFile.read();
		}
		parameter[j] = '\0';
		parameters[i] = atof(parameter);
	}

	// Process the GCode function
	if (!strcmp(functionName, "G00")) {
		move(parameters[0], parameters[1]); // move
	} else if (!strcmp(functionName, "G01")) {
		line(parameters[0], parameters[1]); // draw
	} else if (!strcmp(functionName, "G04")) {
		delay(1000 * parameters[0]); // take a break
	} else if (!strcmp(functionName, "G21") || !strcmp(functionName, "M30")) {
		// Knows but useless GCode functions
	} else {
		// warning(UNKNOWN_GCODE_FUNCTION, functionName); // raise warning
	}
}

//TODO: Gérer l'axe Z
void Drawall::segment(float x, float y, bool isWriting) {
	unsigned long leftTargetLength = positionToLeftLength(
			mDrawingScale * mpScaleX * x + mOffsetX,
			mDrawingScale * mpScaleY * y + mOffsetY);
	unsigned long rightTargetLength = positionToRightLength(
			mDrawingScale * mpScaleX * x + mOffsetX,
			mDrawingScale * mpScaleY * y + mOffsetY);

	// nombre de pas à faire
	long nbPasG = leftTargetLength - mLeftLength;
	long nbPasD = rightTargetLength - mRightLength;

	bool pullLeft = false;
	bool pullRight = false;

	float delaiG;
	float delaiD;

	unsigned long dernierTempsG;
	unsigned long dernierTempsD;

	// calcul de la direction
	if (nbPasG < 0) {
		pullLeft = true;
	}

	if (nbPasD < 0) {
		pullRight = true;
	}
	// On a le sens, donc on peut retirer le signe pour simplifier les calculs
	nbPasG = fabs(nbPasG);
	nbPasD = fabs(nbPasD);

	if (nbPasG > nbPasD) {
		delaiG = mDelay;
		delaiD = mDelay * ((float) nbPasG / (float) nbPasD);
	} else {
		delaiD = mDelay;
		delaiG = mDelay * ((float) nbPasD / (float) nbPasG);
	}

	dernierTempsG = micros();
	dernierTempsD = micros();

	// TODO digitalWrite() should be called only when the direction is changing

	if (pullLeft) {
		digitalWrite(mpReverseMotors ? PIN_RIGHT_MOTOR_DIR : PIN_LEFT_MOTOR_DIR,
				mpLeftDirection);
	} else {
		digitalWrite(mpReverseMotors ? PIN_RIGHT_MOTOR_DIR : PIN_LEFT_MOTOR_DIR,
				!mpLeftDirection);
	}

	if (pullRight) {
		digitalWrite(mpReverseMotors ? PIN_LEFT_MOTOR_DIR : PIN_RIGHT_MOTOR_DIR,
				mpRightDirection);
	} else {
		digitalWrite(mpReverseMotors ? PIN_LEFT_MOTOR_DIR : PIN_RIGHT_MOTOR_DIR,
				!mpRightDirection);
	}

	while (nbPasG > 0 || nbPasD > 0) {
		// Si le delai est franchi et qu'il reste des pas à faire
		if ((nbPasG > 0) && (micros() - dernierTempsG >= delaiG)) {
			dernierTempsG = micros();	// Stoque le temps actuel dans lastTimer
			leftStep(pullLeft);	// Effectue le pas
			nbPasG--;			// Décremente le nb de pas restants
		}

		if ((nbPasD > 0) && (micros() - dernierTempsD >= delaiD)) {
			dernierTempsD = micros();	// stoque le temps actuel dans lastTimer
			nbPasD--;			// decremente le nb de pas restants
			rightStep(pullRight);	// Effectue le pas
		}
	}

	mPositionX = x;
	mPositionY = y;
}

void Drawall::error(SerialData errorNumber) {
#ifdef SERIAL
	Serial.print((byte) errorNumber);
#endif
	//TODO ring buzzer
	delay(1000);
	writingPen(false);
	while (true)
		;
}

void Drawall::warning(SerialData warningNumber) {
#ifdef SERIAL
	Serial.print((byte) warningNumber);
#endif
	//TODO ring buzzer
}

void Drawall::initScale(DrawingSize size) {
	switch (size) {
	case ORIGINAL:
		mDrawingScale = 1;
		break;
	case FULL:
		if (mDrawingWidth / mDrawingHeight > mpSheetWidth / mpSheetHeight) {
			mDrawingScale = (float) mpSheetWidth / mDrawingWidth;
		} else {
			mDrawingScale = (float) mpSheetHeight / mDrawingHeight;
		}
		break;
	default:
		break;
	}
}

void Drawall::initOffset(CardinalPoint position) {
	// write less
	int right = mpSheetWidth - mDrawingScale * mDrawingWidth;
	int up = mpSheetHeight - mDrawingScale * mDrawingHeight;
	int h_center = mpSheetWidth / 2 - mDrawingScale * mDrawingWidth / 2;
	int v_center = mpSheetHeight / 2 - mDrawingScale * mDrawingHeight / 2;

	switch (position) {
	case LOWER_LEFT:
		mOffsetX = mpOffsetX;
		mOffsetY = mpOffsetY;
		break;
	case LOWER_CENTER:
		mOffsetX = h_center + mpOffsetX;
		mOffsetY = mpOffsetY;
		break;
	case LOWER_RIGHT:
		mOffsetX = right + mpOffsetX;
		mOffsetY = mpOffsetY;
		break;

	case LEFT_CENTER:
		mOffsetX = mpOffsetX;
		mOffsetY = v_center + mpOffsetY;
		break;
	case CENTER:
		mOffsetX = h_center + mpOffsetX;
		mOffsetY = v_center + mpOffsetY;
		break;
	case RIGHT_CENTER:
		mOffsetX = right + mpOffsetX;
		mOffsetY = v_center + mpOffsetY;
		break;

	case UPPER_LEFT:
		mOffsetX = mpOffsetX;
		mOffsetY = up + mpOffsetY;
		break;
	case UPPER_CENTER:
		mOffsetX = h_center + mpOffsetX;
		mOffsetY = up + mpOffsetY;
		break;
	case UPPER_RIGHT:
		mOffsetX = right + mpOffsetX;
		mOffsetY = up + mpOffsetY;
		break;

	default:
		break;
	}
}

void Drawall::drawingArea(DrawingSize size, CardinalPoint position) {
	mFile = SD.open(mpDrawingFileName);

	if (!mFile) {
		error(ERR_FILE_NOT_FOUND);
	}

	mDrawingWidth = 25000; // processVar();
	mDrawingHeight = 25000; // processVar();
	initScale(size);
	initOffset(position);

	move(0, 0);
	line(mDrawingWidth, 0);
	line(mDrawingWidth, mDrawingHeight);
	line(0, mDrawingHeight);
	line(0, 0);

	mOffsetX = 0;
	mOffsetY = 0;
	mDrawingHeight = mpSheetHeight; // pour ne plus soustraire la hauteur de l'image

	mFile.close();
}

void Drawall::draw(DrawingSize size, CardinalPoint position) {
	mFile = SD.open(mpDrawingFileName);

	if (!mFile) {
		error(ERR_FILE_NOT_FOUND);
	}

	mDrawingWidth = 25000; // processVar();
	mDrawingHeight = 25000; // processVar();
	initScale(size);
	initOffset(position);

	// Tant qu'on peut lire
	while (mFile.available()) {
		processSDLine();
	}

	mOffsetX = 0;
	mOffsetY = 0;
	mDrawingHeight = mpSheetHeight; // pour ne plus soustraire la hauteur de l'image

	mFile.close();
#ifdef SERIAL
	Serial.print(DRAW_END_DRAWING);
#endif
	end();
}

//int Drawall::processVar() {
//	char car = ' ';
//	char buffer[10];
//	byte i;
//
//	while (car != '=') {
//		car = mFile.read();
//	}
//
//	mFile.read(); // ignore space
//	car = mFile.read();
//
//	for (i = 0; car != '\n'; i++) {
//		buffer[i] = car;
//		car = mFile.read();
//	}
//
//	buffer[i] = '\0';
//	return atoi(buffer);
//}

void Drawall::end() {
	move(mpEndPosition);
	power(false);
	while (true)
		;
}

bool Drawall::atob(char *value) {
	return !strcmp(value, "true") || !strcmp(value, "yes");
}

Drawall::CardinalPoint Drawall::atop(char *str_pos) {
	CardinalPoint pos;

	if (!strcmp(str_pos, "LOWER_LEFT")) {
		pos = LOWER_LEFT;
	} else if (!strcmp(str_pos, "LOWER_CENTER")) {
		pos = LOWER_CENTER;
	} else if (!strcmp(str_pos, "LOWER_RIGHT")) {
		pos = LOWER_RIGHT;
	} else if (!strcmp(str_pos, "LEFT_CENTER")) {
		pos = LEFT_CENTER;
	} else if (!strcmp(str_pos, "CENTER")) {
		pos = CENTER;
	} else if (!strcmp(str_pos, "RIGHT_CENTER")) {
		pos = RIGHT_CENTER;
	} else if (!strcmp(str_pos, "UPPER_LEFT")) {
		pos = UPPER_LEFT;
	} else if (!strcmp(str_pos, "UPPER_CENTER")) {
		pos = UPPER_CENTER;
	} else if (!strcmp(str_pos, "UPPER_RIGHT")) {
		pos = UPPER_RIGHT;
	} else {
		warning(WARN_UNKNOWN_CONFIG_POSITION);
	}

	return pos;
}

void Drawall::loadParameters() {
#define PARAM_BUFFER_SIZE 32	// Taille du buffer
#define NB_PARAMETERS 23		// Nombre de paramètres à lire

	char buffer[PARAM_BUFFER_SIZE];	// Stocke une ligne du fichier
	char *key;					// Chaine pour la clé
	char *value;				// Chaine pour la valeur

	byte i;						// Itérateur
	byte line_length;			// Longueur de la ligne
	byte line_counter = 0;		// Compteur de lignes
	char err_buffer[3];		// Buffer pour affichage numéro de ligne si erreur
	int nb_parsed = 0;

	// Ici, problème avec la communication avec Processing !!!

	// Test existence fichier
	/*if(!SD.exists(fileName)) {
	 * Serial.print("Le fichier '");
	 * Serial.print(fileName);
	 * Serial.println("' n'existe pas.");
	 * while(true){};
	 * } */

	// Ouvre le fichier de configuration
	File configFile = SD.open(CONFIG_FILE_NAME, FILE_READ);

	if (!configFile) {
		error(ERR_FILE_NOT_READABLE);
	}
	// Tant qu'on est pas à la fin du fichier
	while (configFile.available() > 0) {
		// Récupère une ligne entière dans le buffer
		i = 0;
		while ((buffer[i++] = configFile.read()) != '\n') {

			// Si la ligne dépasse la taille du buffer
			if (i == PARAM_BUFFER_SIZE) {
				// On finit de lire la ligne mais sans stocker les données
				while (configFile.read() != '\n') {
				};
				break;			// Et on arrête la lecture de cette ligne
			}
		}

		// On garde de côté le nombre de char stocké dans le buffer
		line_length = i;

		// Finalise la chaine de caractéres ASCII en supprimant le \n au passage.
		buffer[--i] = '\0';

		++line_counter;			// Incrémente le compteur de lignes

		// Ignore les lignes vides ou les lignes de commentaires.
		if (buffer[0] == '\0' || buffer[0] == '#') {
			continue;
		}
		// Gestion des lignes trop grande
		if (i == PARAM_BUFFER_SIZE) {
			sprintf(err_buffer, "%d", line_counter);
			warning(WARN_TOO_LONG_CONFIG_LINE);
		}
		// Cherche l'emplacement de la clé en ignorant les espaces et les tabulations en début de ligne.
		i = 0;
		// Ignore les lignes contenant uniquement des espaces et/ou des tabulations.
		while (buffer[i] == ' ' || (buffer[i] == '\t' && !(++i == line_length)))
			;

		if (i == line_length) {
			continue;// Gère les lignes contenant uniquement des espaces et/ou des tabulations.
		}
		key = &buffer[i];

		// Cherche l'emplacement du séparateur = en ignorant les espaces et les tabulations apres la clé.
		while (buffer[i] != ' ' && buffer[i] != '\t') {
			if (++i == line_length) {
				sprintf(err_buffer, "%d", line_counter);
				warning(WARN_WRONG_CONFIG_LINE);
				break;			// Ignore les lignes mal formées
			}
		}

		if (i == line_length) {
			continue;			// Gère les lignes mal formées
		}
		buffer[i++] = '\0';		// Transforme le séparateur en \0

		// Cherche l'emplacement de la valeur en ignorant les espaces et les tabulations après le séparateur.
		while (buffer[i] == ' ' || buffer[i] == '\t') {
			if (++i == line_length) {
				sprintf(err_buffer, "%d", line_counter);
				warning(WARN_WRONG_CONFIG_LINE);
				break;			// Ignore les lignes mal formées
			}
		}

		if (i == line_length) {
			continue;			// Gère les lignes mal formées
		}

		value = &buffer[i];

		// TODO use constants for the param names

		// Transforme les données texte en valeur utilisable
		if (!strcmp(key, "span")) {
			mpSpan = atoi(value);
		} else if (!strcmp(key, "sheetWidth")) {
			mpSheetWidth = atoi(value);
		} else if (!strcmp(key, "sheetHeight")) {
			mpSheetHeight = atoi(value);
		} else if (!strcmp(key, "sheetPositionX")) {
			mpSheetPositionX = atoi(value);
		} else if (!strcmp(key, "sheetPositionY")) {
			mpSheetPositionY = atoi(value);
		} else if (!strcmp(key, "servoWritingAngle")) {
			mpServoWritingAngle = atoi(value);
		} else if (!strcmp(key, "servoMoovingAngle")) {
			mpServoMoovingAngle = atoi(value);
		} else if (!strcmp(key, "preServoDelay")) {
			mpPreServoDelay = atoi(value);
		} else if (!strcmp(key, "postServoDelay")) {
			mpPostServoDelay = atoi(value);
		} else if (!strcmp(key, "steps")) {
			mpSteps = atoi(value);
		} else if (!strcmp(key, "stepMode")) {
			mpStepMode = atoi(value);
		} else if (!strcmp(key, "diameter")) {
			mpDiameter = atoi(value);
		} else if (!strcmp(key, "leftDirection")) {
			mpLeftDirection = atob(value);
		} else if (!strcmp(key, "rightDirection")) {
			mpRightDirection = atob(value);
		} else if (!strcmp(key, "reverseMotors")) {
			mpReverseMotors = atob(value);
		} else if (!strcmp(key, "initialDelay")) {
			mpInitialDelay = atoi(value);
		} else if (!strcmp(key, "scaleX")) {
			mpScaleX = atof(value);
		} else if (!strcmp(key, "scaleY")) {
			mpScaleY = atof(value);
		} else if (!strcmp(key, "offsetX")) {
			mpOffsetX = atoi(value);
		} else if (!strcmp(key, "offsetY")) {
			mpOffsetY = atoi(value);
		} else if (!strcmp(key, "defaultSpeed")) {
			mpDefaultSpeed = atoi(value);
		} else if (!strcmp(key, "initPosition")) {
			mpInitPosition = atop(value);
		} else if (!strcmp(key, "endPosition")) {
			mpEndPosition = atop(value);
		} else if (!strcmp(key, "drawingFile")) {
			strcpy(mpDrawingFileName, value);
			// TODO add drawingPosition(CardinalPoint)
		} else {
			configFile.close();
			warning(WARN_UNKNOWN_CONFIG_KEY);
		}
		nb_parsed++;
	}

	configFile.close();			// Ferme le fichier de configuration

	if (nb_parsed < NB_PARAMETERS) {
		error(ERR_TOO_FEW_PARAMETERS);
	} else if (nb_parsed > NB_PARAMETERS) {
		error(ERR_TOO_MANY_PARAMETERS);
	}

}

