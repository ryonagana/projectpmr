#include "ItemActions.h"

ItemStorage* ItemActions::last_inventory = 0;
TextItem* ItemActions::last_src = 0;
unsigned char ItemActions::last_id = 0;

void ItemActions::UseItem(ItemStorage* inventory, TextItem* src, unsigned char id)
{
	last_inventory = inventory;
	last_src = src;
	last_id = id;

	unsigned char usage = ResourceCache::GetItemUse(id);
	//anything 0x3F and under is outside-only
	//anything between 0x40 and 0x7F can be used both places
	//anything between 0x80 and 0xBF is battle-only
	//anything 0xC0 and above is safari-only
	unsigned char state = Engine::GetState();

	if (usage <= 0x3F && state != States::OVERWORLD)
		usage = 0;
	else if (usage >= 0x40 && usage <= 0x7F && state == States::SAFARIBATTLE)
		usage = 0;
	else if (usage >= 0x80 && usage < 0xC0 && state != States::BATTLE)
		usage = 0;
	else if (usage >= 0xC0 && state != States::SAFARIBATTLE)
		usage = 0;

	auto close_pokemon = [inventory](TextItem* src)
	{
		if (!MenuCache::PokemonMenu()->GetChooseTextbox()->SetToClose())
			MenuCache::PokemonMenu()->GetChooseTextbox()->Close();
		inventory->GetMenu()->GetTextboxes()[0]->Close();
	};

	Textbox* t;
	string s;
	unsigned char v = 0;
	bool able[6] = {};
	switch (usage)
	{
	case 0:
		t = new Textbox();
		s = string(pokestring("OAK: ")).append(inventory->GetOwner()->GetName()).append(pokestring("!\nThis isn't the\vtime to use that!\f"));
		t->SetText(new TextItem(t, [inventory](TextItem* src)
		{
			inventory->GetMenu()->GetTextboxes()[0]->Close();
		}, s));
		src->GetParent()->ShowTextbox(t, false);
		break;

	case 0x05: //evostone
		for (int i = 0; i < 6; i++)
		{
			if (!Players::GetPlayer1()->GetParty()[i])
				break;
			for (int e = 0; e < 5; e++)
			{
				if (!Players::GetPlayer1()->GetParty()[i]->evolutions[e].trigger)
					break;
				Pokemon* p = Players::GetPlayer1()->GetParty()[i];
				if (p->evolutions[e].trigger == EVOLUTION_ITEM && Players::GetPlayer1()->GetParty()[i]->evolutions[e].item == id)
				{
					able[i] = true;
					break;
				}
			}
		}
		MenuCache::PokemonMenu()->SetAbleNotAble(able);

		MenuCache::PokemonMenu()->UpdatePokemon(Players::GetPlayer1()->GetParty(), true);
		MenuCache::PokemonMenu()->Show(src->GetParent(), pokestring("Use item on which\n\n#MON?"), UseEvoStone, close_pokemon);
		break;

	case 0x07: //repels
		if (id == ITEM_REPEL)
			v = REPEL_STEPS;
		else if (id == ITEM_SUPER_REPEL)
			v = SUPER_REPEL_STEPS;
		else if (id == ITEM_MAX_REPEL)
			v = MAX_REPEL_STEPS;

		if (state == States::OVERWORLD) //we must check in case someone modifies the item usage to work in battle
			((MapScene*)Engine::GetActiveScene())->SetRepel(v);
		t = new Textbox();
		s = string(inventory->GetOwner()->GetName()).append(pokestring(" used\n")).append(ResourceCache::GetItemName(id)).append(pokestring("!\f"));
		t->SetText(new TextItem(t, [inventory](TextItem* src) { inventory->GetMenu()->GetTextboxes()[0]->Close(); inventory->RemoveItemFromSlot(last_src->index, 1); }, s));
		src->GetParent()->ShowTextbox(t, false);
		break;

	case 0x08: //vitamin
		MenuCache::PokemonMenu()->UpdatePokemon(Players::GetPlayer1()->GetParty());
		MenuCache::PokemonMenu()->Show(src->GetParent(), pokestring("Use item on which\n\n#MON?"), UseVitamin, close_pokemon);
		break;

	case 0x40: //medicine
		MenuCache::PokemonMenu()->UpdatePokemon(Players::GetPlayer1()->GetParty());
		MenuCache::PokemonMenu()->Show(src->GetParent(), pokestring("Use item on which\n\n#MON?"), UseMedicine, close_pokemon);
		break;

	default:
		t = new Textbox();
		s = pokestring("Sorry, that item\nhasn't been\vimplemented yet.");
		t->SetText(new TextItem(t, [inventory](TextItem* src)
		{
			inventory->GetMenu()->GetTextboxes()[0]->Close();
		}, s));
		src->GetParent()->ShowTextbox(t, false);
		break;
	}
}

void ItemActions::UseMedicine(TextItem* src)
{
	Pokemon* p = MenuCache::PokemonMenu()->GetParty()[src->index];
	ItemStorage* items = last_inventory;

	auto heal_callback = [p](TextItem* src)
	{
		MenuCache::PokemonMenu()->GetChooseTextbox()->GetItems()[0]->SetText(string(p->nickname).append(pokestring("\n\nrecovered by ").append(pokestring(itos(MenuCache::PokemonMenu()->GetHPAmountChanged()).c_str())).append(pokestring("!"))));
		MenuCache::PokemonMenu()->UpdateOnePokemon(src->index);
		MenuCache::PokemonMenu()->GetChooseTextbox()->UpdateMenu();
		MenuCache::PokemonMenu()->FocusChooseTextbox();
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
	};

	auto revive_callback = [p](TextItem* src)
	{
		p->status = Statuses::OK;
		MenuCache::PokemonMenu()->GetChooseTextbox()->GetItems()[0]->SetText(string(p->nickname).append(pokestring("\n\nis revitalized!")));
		MenuCache::PokemonMenu()->UpdateOnePokemon(src->index);
		MenuCache::PokemonMenu()->GetChooseTextbox()->UpdateMenu();
		MenuCache::PokemonMenu()->FocusChooseTextbox();
		MenuCache::PokemonMenu()->GetMenu()->UpdateMenu();
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
	};

	switch (last_id)
	{
	case 0x0B: //antidote
		if (p->status != Statuses::POISONED)
			break;
		p->status = Statuses::OK;
		MenuCache::PokemonMenu()->GetChooseTextbox()->GetItems()[0]->SetText(string(p->nickname).append(pokestring(" was\n\ncured of poison!")));
		MenuCache::PokemonMenu()->UpdatePokemon(MenuCache::PokemonMenu()->GetParty());
		MenuCache::PokemonMenu()->GetChooseTextbox()->UpdateMenu();
		MenuCache::PokemonMenu()->FocusChooseTextbox();
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x0C: //burn heal
		if (p->status != Statuses::BURNED)
			break;
		p->status = Statuses::OK;
		MenuCache::PokemonMenu()->GetChooseTextbox()->GetItems()[0]->SetText(string(p->nickname).append(pokestring("'s\n\nburn was healed!")));
		MenuCache::PokemonMenu()->UpdatePokemon(MenuCache::PokemonMenu()->GetParty());
		MenuCache::PokemonMenu()->GetChooseTextbox()->UpdateMenu();
		MenuCache::PokemonMenu()->FocusChooseTextbox();
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x0D: //ice heal
		if (p->status != Statuses::FROZEN)
			break;
		p->status = Statuses::OK;
		MenuCache::PokemonMenu()->GetChooseTextbox()->GetItems()[0]->SetText(string(p->nickname).append(pokestring(" was\n\ndefrosted!")));
		MenuCache::PokemonMenu()->UpdatePokemon(MenuCache::PokemonMenu()->GetParty());
		MenuCache::PokemonMenu()->GetChooseTextbox()->UpdateMenu();
		MenuCache::PokemonMenu()->FocusChooseTextbox();
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x0E: //awakening
		if (p->status != Statuses::SLEEPING)
			break;
		p->status = Statuses::OK;
		MenuCache::PokemonMenu()->GetChooseTextbox()->GetItems()[0]->SetText(string(p->nickname).append(pokestring("\n\nwoke up!")));
		MenuCache::PokemonMenu()->UpdatePokemon(MenuCache::PokemonMenu()->GetParty());
		MenuCache::PokemonMenu()->GetChooseTextbox()->UpdateMenu();
		MenuCache::PokemonMenu()->FocusChooseTextbox();
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x0F: //paralyze heal
		if (p->status != Statuses::PARALYZED)
			break;
		p->status = Statuses::OK;
		MenuCache::PokemonMenu()->GetChooseTextbox()->GetItems()[0]->SetText(string(p->nickname).append(pokestring("'s\n\nrid of paralysis!")));
		MenuCache::PokemonMenu()->UpdatePokemon(MenuCache::PokemonMenu()->GetParty());
		MenuCache::PokemonMenu()->GetChooseTextbox()->UpdateMenu();
		MenuCache::PokemonMenu()->FocusChooseTextbox();
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x34: //full heal
		if (p->status == Statuses::OK || p->status == Statuses::FAINTED)
			break;
		p->status = Statuses::OK;
		MenuCache::PokemonMenu()->GetChooseTextbox()->GetItems()[0]->SetText(string(p->nickname).append(pokestring("'s\n\nhealth returned!")));
		MenuCache::PokemonMenu()->UpdatePokemon(MenuCache::PokemonMenu()->GetParty());
		MenuCache::PokemonMenu()->GetChooseTextbox()->UpdateMenu();
		MenuCache::PokemonMenu()->FocusChooseTextbox();
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x35: //revive
		if (p->hp > 0)
			break;
		MenuCache::PokemonMenu()->Heal(p->max_hp / 2, revive_callback);
		return;

	case 0x36: //max revive
		if (p->hp > 0)
			break;
		MenuCache::PokemonMenu()->Heal(p->max_hp, revive_callback);
		return;

	case 0x10: //full restore
		if (p->hp == p->max_hp)
		{
			if (p->status == Statuses::OK)
				break;
			p->status = Statuses::OK;
			MenuCache::PokemonMenu()->GetChooseTextbox()->GetItems()[0]->SetText(string(p->nickname).append(pokestring("'s\n\nhealth returned!")));
			MenuCache::PokemonMenu()->UpdatePokemon(MenuCache::PokemonMenu()->GetParty());
			MenuCache::PokemonMenu()->GetChooseTextbox()->UpdateMenu();
			MenuCache::PokemonMenu()->FocusChooseTextbox();
			last_inventory->RemoveItemFromSlot(last_src->index, 1);
			return;
		}
		//we need a slightly different callback to restore the status at the end
		MenuCache::PokemonMenu()->Heal(p->max_hp - p->hp, [p](TextItem* src)
		{
			MenuCache::PokemonMenu()->GetChooseTextbox()->GetItems()[0]->SetText(string(p->nickname).append(pokestring("\n\nrecovered by ").append(pokestring(itos(MenuCache::PokemonMenu()->GetHPAmountChanged()).c_str())).append(pokestring("!"))));
			p->status = Statuses::OK;
			MenuCache::PokemonMenu()->UpdateOnePokemon(src->index);
			MenuCache::PokemonMenu()->GetMenu()->UpdateMenu();
			MenuCache::PokemonMenu()->GetChooseTextbox()->UpdateMenu();
			MenuCache::PokemonMenu()->FocusChooseTextbox();
			last_inventory->RemoveItemFromSlot(last_src->index, 1);
		});
		return;

	case 0x11: //max potion
		if (p->hp == p->max_hp)
			break;
		MenuCache::PokemonMenu()->Heal(p->max_hp - p->hp, heal_callback);
		return;

	case 0x12: //hyper potion
		if (p->hp == p->max_hp)
			break;
		MenuCache::PokemonMenu()->Heal(200, heal_callback);
		return;

	case 0x13: //super potion
		if (p->hp == p->max_hp)
			break;
		MenuCache::PokemonMenu()->Heal(50, heal_callback);
		return;

	case 0x14: //potion
		if (p->hp == p->max_hp)
			break;
		MenuCache::PokemonMenu()->Heal(20, heal_callback);
		return;
	}

	Textbox* t = new Textbox();
	t->SetText(new TextItem(src->GetParent(), [items](TextItem* s){MenuCache::PokemonMenu()->GetMenu()->Close(); }, pokestring("It won't have any\n\neffect.")));
	MenuCache::PokemonMenu()->GetMenu()->SetArrowState(ArrowStates::INACTIVE);
	src->GetParent()->ShowTextbox(t, false);
}

void ItemActions::UseVitamin(TextItem* src)
{
	Pokemon* p = MenuCache::PokemonMenu()->GetParty()[src->index];
	ItemStorage* items = last_inventory;

	unsigned int stat = 0;
	Textbox* t;
	switch (last_id)
	{
	case 0x23: //hp up
		if (p->ev_hp >= 25600)
		break;
		p->ev_hp = min(p->ev_hp + EV_MAXVITAMIN / 10, EV_MAXVITAMIN);
		stat = p->max_hp;
		p->RecalculateStats();
		p->hp += p->max_hp - stat;

		t = new Textbox();
		t->SetText(new TextItem(src->GetParent(), [items](TextItem* s){MenuCache::PokemonMenu()->GetMenu()->Close(); }, string(p->nickname).append(pokestring("'s\n\nHEALTH rose.\f"))));
		MenuCache::PokemonMenu()->GetMenu()->SetArrowState(ArrowStates::INACTIVE);
		src->GetParent()->ShowTextbox(t, false);
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x24: //protein
		if (p->ev_hp >= 25600)
			break;
		p->ev_attack = min(p->ev_attack + EV_MAXVITAMIN / 10, EV_MAXVITAMIN);
		p->RecalculateStats();
		t = new Textbox();
		t->SetText(new TextItem(src->GetParent(), [items](TextItem* s){MenuCache::PokemonMenu()->GetMenu()->Close(); }, string(p->nickname).append(pokestring("'s\n\nATTACK rose.\f"))));
		MenuCache::PokemonMenu()->GetMenu()->SetArrowState(ArrowStates::INACTIVE);
		src->GetParent()->ShowTextbox(t, false);
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x25: //iron
		if (p->ev_defense >= 25600)
			break;
		p->ev_defense = min(p->ev_defense + EV_MAXVITAMIN / 10, EV_MAXVITAMIN);
		p->RecalculateStats();
		t = new Textbox();
		t->SetText(new TextItem(src->GetParent(), [items](TextItem* s){MenuCache::PokemonMenu()->GetMenu()->Close(); }, string(p->nickname).append(pokestring("'s\n\nDEFENSE rose.\f"))));
		MenuCache::PokemonMenu()->GetMenu()->SetArrowState(ArrowStates::INACTIVE);
		src->GetParent()->ShowTextbox(t, false);
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x26: //carbos
		if (p->ev_speed >= 25600)
			break;
		p->ev_speed = min(p->ev_speed + EV_MAXVITAMIN / 10, EV_MAXVITAMIN);
		p->RecalculateStats();
		t = new Textbox();
		t->SetText(new TextItem(src->GetParent(), [items](TextItem* s){MenuCache::PokemonMenu()->GetMenu()->Close(); }, string(p->nickname).append(pokestring("'s\n\nSPEED rose.\f"))));
		MenuCache::PokemonMenu()->GetMenu()->SetArrowState(ArrowStates::INACTIVE);
		src->GetParent()->ShowTextbox(t, false);
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x27: //calcium
		if (p->ev_special >= 25600)
			break;
		p->ev_special = min(p->ev_special + EV_MAXVITAMIN / 10, EV_MAXVITAMIN);
		p->RecalculateStats();
		t = new Textbox();
		t->SetText(new TextItem(src->GetParent(), [items](TextItem* s){MenuCache::PokemonMenu()->GetMenu()->Close(); }, string(p->nickname).append(pokestring("'s\n\nSPECIAL rose.\f"))));
		MenuCache::PokemonMenu()->GetMenu()->SetArrowState(ArrowStates::INACTIVE);
		src->GetParent()->ShowTextbox(t, false);
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;

	case 0x28: //rare candy
		if (p->level == 100)
			break;

		PokemonUtils::Levelup(src, p, [](TextItem* s) {MenuCache::PokemonMenu()->GetChooseTextbox()->Close(); });
		//MenuCache::PokemonMenu()->GetChooseTextbox()->SetCloseCallback(stats);
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;
	}

	t = new Textbox();
	t->SetText(new TextItem(src->GetParent(), [items](TextItem* s){MenuCache::PokemonMenu()->GetMenu()->Close(); }, pokestring("It won't have any\n\neffect.")));
	MenuCache::PokemonMenu()->GetMenu()->SetArrowState(ArrowStates::INACTIVE);
	src->GetParent()->ShowTextbox(t, false);
}

void ItemActions::UseEvoStone(TextItem* src)
{
	Pokemon* p = MenuCache::PokemonMenu()->GetParty()[src->index];
	ItemStorage* items = last_inventory;

	unsigned char p_into = p->id;
	for (int e = 0; e < 5; e++)
	{
		if (!p->evolutions[e].trigger)
			break;
		if (p->evolutions[e].trigger == EVOLUTION_ITEM && p->evolutions[e].item == last_id)
		{
			p_into = p->evolutions[e].pokemon;
			break;
		}
	}

	if (MenuCache::PokemonMenu()->GetAblility()[MenuCache::PokemonMenu()->GetMenu()->GetActiveIndex()])
	{
		PokemonUtils::Evolve(src->GetParent(), p, p_into)(src);
		last_inventory->RemoveItemFromSlot(last_src->index, 1);
		return;
	}

	Textbox* t = new Textbox();
	t->SetText(new TextItem(src->GetParent(), [items](TextItem* s){MenuCache::PokemonMenu()->GetMenu()->Close(); }, pokestring("It won't have any\n\neffect.")));
	MenuCache::PokemonMenu()->GetMenu()->SetArrowState(ArrowStates::INACTIVE);
	src->GetParent()->ShowTextbox(t, false);
}