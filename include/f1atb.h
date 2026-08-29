#pragma once
#include <Arduino.h>

void pollF1ATB();

// Commande l’action F1ATB configurée (NumAction 0..15) via /ForceAction.
// Le NumAction est entièrement configurable : aucune action n’est codée en dur.
bool stepF1ATBOn();
bool stepF1ATBOff();
// Annule tout forçage en cours et rend la main au mode automatique F1ATB.
bool cancelF1ATBForce();
