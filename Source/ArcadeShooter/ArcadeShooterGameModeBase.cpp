// Copyright Epic Games, Inc. All Rights Reserved.

#include "ArcadeShooterGameModeBase.h"


AArcadeShooterGameModeBase::AArcadeShooterGameModeBase()
{
	DefaultPawnClass = AShip::StaticClass();
}

void AArcadeShooterGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsValid(EnemySpawner) && EnemySpawner != nullptr) {
		EnemySpawner = nullptr;
	}

	if (!bLevelHasEnded) {
		if (bDeathScreenOn) {
			HideDeathScreen();
		}
		else if(bLevelFinishedScreenOn){
			HideLevelFinishedScreen();
		}
	}
}

void AArcadeShooterGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	LoadLevelProgress();
}

void AArcadeShooterGameModeBase::IncrementScore(int Delta)
{
	CurrLevelScore += Delta;
}

void AArcadeShooterGameModeBase::IncrementGalaxyPoints(int Delta)
{
	GalaxyPoints += Delta;
}

FString AArcadeShooterGameModeBase::GetWaveText()
{
	FString Result = "";

	if (IsValid(EnemySpawner)) {

		if (PreviousWaveCount == EnemySpawner->GetCurrWaveCount()) {
			if (EnemySpawner->bLevelFinished) {
				ChangeLevel();
			}
			return Result;
		}
		else {
			if (!GetWorldTimerManager().IsTimerActive(MemberTimerHandle)) {
				GetWorldTimerManager().SetTimer(MemberTimerHandle, this,
					&AArcadeShooterGameModeBase::FinishDisplayingWave, 7.0f, false, 7.0f);
			}
		}

		if (Level < TotalLevels - 1) {
			Result = "Wave " + FString::FromInt(EnemySpawner->GetCurrWaveCount() + 1);
		}
		else if(Level == TotalLevels - 1){
			Result = "Boss";
		}
		else {
			Result = "Survive!";
		}
	}

	return Result;
}

void AArcadeShooterGameModeBase::ChangeLevel()
{
	EndLevel();
	if (Level + 1 >= TotalLevels) {
		ShowCreditsScreen();
	}
	else {
		ShowLevelFinishedScreen();
	}
	Level++;
}

void AArcadeShooterGameModeBase::CalculateScore(int PlanetHealth)
{
	TotalScore += CurrLevelScore;

	if (!bShouldResetScore) {
		TotalScore += PlanetHealth * 600;

		if (GalaxyPoints > 0) {
			TotalScore += GalaxyPoints * 10;
			GalaxyPoints = 0;
		}
		if (PlayerShipsCount > 0) {
			TotalScore += PlayerShipsCount * 500;
			PlayerShipsCount = 0;
		}
	}
}

AShip* AArcadeShooterGameModeBase::SpawnNewPlayerShip(int CurrentShipsCount)
{
	NotifyEnemySpawner(CurrentShipsCount + 1);

	AShip* NewPlayerShip = nullptr;

	TArray<AActor*> FoundPlanets;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlanet::StaticClass(), FoundPlanets);
	if (FoundPlanets.Num() > 0) {
		APlanet* Planet = Cast<APlanet>(FoundPlanets[0]);
		if (IsValid(Planet)) {
			NewPlayerShip = GetWorld()->SpawnActor<AShip>(PlayerClass,
				FVector(0, Planet->Radius, 0),
				FRotator(0, 0, 0));
		}
	}

	return NewPlayerShip;
}

APlayerShipProjection* AArcadeShooterGameModeBase::SpawnPlayerShipProjection()
{
	TArray<AActor*> FoundPlanets;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlanet::StaticClass(), FoundPlanets);
	APlanet* Planet = Cast<APlanet>(FoundPlanets[0]);

	return GetWorld()->SpawnActor<APlayerShipProjection>(PlayerProjectionClass,
		FVector(0, Planet->Radius, 0),
		FRotator(0, 90, 0)
		);
}

void AArcadeShooterGameModeBase::ShowShipMessage(FVector Location, int ShipLevel)
{
	APopUpMessage* Message = GetWorld()->SpawnActor<APopUpMessage>(PopUpMessageClass,
		Location,
		FRotator(180, 0, 180));
	if (IsValid(Message)) {
		FString ShipLevelText = "";
		if (ShipLevel == 3) {
			ShipLevelText = "Max Level";
		}
		else {
			ShipLevelText = "Level " + FString::FromInt(ShipLevel);
		}
		Message->SetTexts(ShipLevelText, "-400");
		Message->SetColor(true, Message->ScoreColor);
		Message->SetColor(false, Message->GPColor);

		if (ShipLevel > 1) {
			PlayUpgradeSound(Location);
		}
		else {
			PlayNewShipSound(Location);
		}
	}
}

void AArcadeShooterGameModeBase::StartLevel()
{
	bNewHighscore = false;
	CurrLevelScore = 0;
	GalaxyPoints = 0;
	EndLevel();
	bLevelHasEnded = false;
	bShouldResetScore = false;

	EnemySpawner = Cast<AEnemySpawner>(GetWorld()->SpawnActor(EnemySpawnerClass));
	NotifyEnemySpawner(1);

	APlanet* Planet = GetWorld()->SpawnActor<APlanet>(PlanetClass, 
									FVector(0,0,0), 
									FRotator(0,0,0));

	PlayBackgroundMusic();
}

void AArcadeShooterGameModeBase::StartPlay()
{
	Super::StartPlay();

	if (Level < TotalLevels || Level == 100) {
		StartLevel();
	}
}

void AArcadeShooterGameModeBase::FinishDisplayingWave()
{
	if (IsValid(EnemySpawner)) {
		PreviousWaveCount = EnemySpawner->GetCurrWaveCount();
	}
}

void AArcadeShooterGameModeBase::EndLevel()
{
	TArray<AActor*> FoundPlanets;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlanet::StaticClass(), FoundPlanets);
	if (FoundPlanets.Num() > 0) {
		APlanet* Planet = Cast<APlanet>(FoundPlanets[0]);

		if (IsValid(Planet)) {
			CalculateScore(Planet->Health);
		}
	}
	else {
		CalculateScore(0);
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), 
										AActor::StaticClass(), 
										FoundActors);

	for (AActor* Actor : FoundActors) {
		if (IsValid(Cast<AShip>(Actor)) || 
			IsValid(Cast<AProjectile>(Actor)) ||
			IsValid(Cast<AEnemySpawner>(Actor)) || 
			IsValid(Cast<APlanet>(Actor)) ||
			IsValid(Cast<AShip>(Actor)) || 
			IsValid(Cast<ADrop>(Actor)) ||
			IsValid(Cast<AIndicator>(Actor)) || 
			IsValid(Cast<APlayerShipProjection>(Actor)) ||
			IsValid(Cast<APopUpMessage>(Actor)))
		{
			Actor->Destroy();
		}
	}

	StopPlayingBackgroundMusic();
	PlayerShipsCount = 0;
	bLevelHasEnded = true;
	SaveHighscore();
	SaveLevelProgress(false);
}

void AArcadeShooterGameModeBase::NotifyEnemySpawner(int NewPlayerShipsCount)
{
	PlayerShipsCount = NewPlayerShipsCount;
	if (IsValid(EnemySpawner)) {
		EnemySpawner->PlayerShipsCount = PlayerShipsCount;
	}
}

void AArcadeShooterGameModeBase::ResetScore()
{
	bShouldResetScore = true;
}

void AArcadeShooterGameModeBase::PlayPlayerFiringSound(WeaponType Weapon, FVector Location, float VolumeMult)
{
	switch (Weapon) {
	case WeaponType::Rapid:
			PlayRapidFireSound(Location, VolumeMult);
		break;
	case WeaponType::Radial:
			PlayRadialFireSound(Location, VolumeMult);
		break;
	case WeaponType::Frost:
			PlayFrostFireSound(Location, VolumeMult);
		break;
	default:
		break;
	}
}

void AArcadeShooterGameModeBase::SaveHighscore()
{
	FString SlotName = "Highscore";
	uint32 UserIndex = 0;

	if (UPTPSaveGame* LoadedGame = Cast<UPTPSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex)))
	{
		if (LoadedGame->Score > TotalScore) {
			return;
		}
	}

	if (UPTPSaveGame* SaveGameInstance = Cast<UPTPSaveGame>(UGameplayStatics::CreateSaveGameObject(UPTPSaveGame::StaticClass())))
	{
		bNewHighscore = true;

		SaveGameInstance->Score = TotalScore;

		UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName, UserIndex);
	}
}

void AArcadeShooterGameModeBase::SaveLevelProgress(bool bNewGame)
{
	FString SlotName = "LevelProgress";
	uint32 UserIndex = 0;

	if (Level == 100)
		return;

	if (UPTPSaveGame* SaveGameInstance = Cast<UPTPSaveGame>(UGameplayStatics::CreateSaveGameObject(UPTPSaveGame::StaticClass())))
	{
		if (bNewGame) {
			SaveGameInstance->Level = 0;
			SaveGameInstance->Score = 0;
		}
		else {
			SaveGameInstance->Level = Level;
			SaveGameInstance->Score = TotalScore;
		}

		UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName, UserIndex);
	}
}

void AArcadeShooterGameModeBase::LoadLevelProgress()
{
	FString SlotName = "LevelProgress";
	uint32 UserIndex = 0;

	if (UPTPSaveGame* LoadedGame = Cast<UPTPSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex)))
	{
		if (LoadedGame->bHighscoreModeOn) {
			LoadedGame->bHighscoreModeOn = false;
			UGameplayStatics::SaveGameToSlot(LoadedGame, SlotName, UserIndex);
			TotalScore = 0;
			Level = 100;
		}
		else {
			TotalScore = LoadedGame->Score;
			Level = LoadedGame->Level;
		}
	}
	else {
		TotalScore = 0;
		Level = 0;
	}
}