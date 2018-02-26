// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "FeedbackFileEditor.h"

#include "IPluginManager.h"
#include "SlateCore.h"
#include "SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "FFeedbackFileEditorModule"

void FFeedbackFileEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	StyleSet = MakeShareable(new FSlateStyleSet("FeedbackFileStyle"));

	//Content path of this plugin
	FString ContentDir = IPluginManager::Get().FindPlugin("HapticsManager")->GetBaseDir();

	//The image we wish to load is located inside the Resources folder inside the Base Directory
	//so let's set the content dir to the base dir and manually switch to the Resources folder:
	StyleSet->SetContentRoot(ContentDir);

	//Create a brush from the icon
	FSlateImageBrush* ThumbnailBrush = new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Resources/bat_on"), TEXT(".png")), FVector2D(128.f, 128.f));
	FSlateImageBrush* ThumbnailBrushArm = new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Resources/tosy_r_on"), TEXT(".png")), FVector2D(128.f, 128.f));
	FSlateImageBrush* ThumbnailBrushHead = new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Resources/tal_on"), TEXT(".png")), FVector2D(128.f, 128.f));
	FSlateImageBrush* ThumbnailBrushBody = new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Resources/ot_on"), TEXT(".png")), FVector2D(128.f, 128.f));

	if (ThumbnailBrush)
	{
		//In order to bind the thumbnail to our class we need to type ClassThumbnail.X where X is the name of the C++ class of the asset
		StyleSet->Set("ClassThumbnail.FeedbackFile", ThumbnailBrush);
	}

	if (ThumbnailBrushBody)
	{
		StyleSet->Set("ClassThumbnail.TactotFeedbackFile", ThumbnailBrushBody);
	}

	if (ThumbnailBrushArm)
	{
		StyleSet->Set("ClassThumbnail.TactosyFeedbackFile", ThumbnailBrushArm);
	}

	if (ThumbnailBrushHead)
	{
		StyleSet->Set("ClassThumbnail.TactalFeedbackFile", ThumbnailBrushHead);
	}

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FFeedbackFileEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FSlateStyleRegistry::UnRegisterSlateStyle(StyleSet->GetStyleSetName());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFeedbackFileEditorModule, FeedbackFileEditor)