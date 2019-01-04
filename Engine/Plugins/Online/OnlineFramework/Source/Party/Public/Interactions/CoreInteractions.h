// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SocialInteractionMacros.h"

/**
 * Core set of user/friend interactions supported by the Social framework
 */

DECLARE_SOCIAL_INTERACTION_EXPORT(PARTY_API, AddFriend);
DECLARE_SOCIAL_INTERACTION_EXPORT(PARTY_API, RemoveFriend);
DECLARE_SOCIAL_INTERACTION_EXPORT(PARTY_API, AcceptFriendInvite);
DECLARE_SOCIAL_INTERACTION_EXPORT(PARTY_API, RejectFriendInvite);

DECLARE_SOCIAL_INTERACTION_EXPORT(PARTY_API, Block);
DECLARE_SOCIAL_INTERACTION_EXPORT(PARTY_API, Unblock);
DECLARE_SOCIAL_INTERACTION_EXPORT(PARTY_API, PrivateMessage);

DECLARE_SOCIAL_INTERACTION_EXPORT(PARTY_API, ShowPlatformProfile);