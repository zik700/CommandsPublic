<?php
/*
 * Opis
 *
 * @since 06.03.2019 , eDokumenty ver. 5.4.36
 * @author Damian Gryń
 * @version 2.0.0
 * @copyright Copyright (c) eDokumenty Sp. z
 */
require_once('./classes/eDokumentyApi/EDokApi.inc');
require_once(MOD_PATH.'CRegisters/beans/CRegisterEntry.inc');
ini_set('memory_limit', -1);
set_time_limit(400);
 class GenerateRatings{

    const DEF_COLUMNS_RAITING = array('prac__', 'akcept', 'doc_id' , 'rodzaj');

        /**
     * Typy oceny rocznej
     * 2 - Ocena kadry menadżerskiej
     * 1 - Ocena roczna
     */
    //Typy oceny rocznej
    const TYPE_YEAR_RATING = '1';
    const TYPE_MANAGER_RATING = '2';

    // cecha dla oceny rocznej - Pracownik
    const FEATID_NEW_RATING = '319';
    const FEATID_MANAGER_RATING = '296';

    /**
     * Typy dokumentów
     * 47 - Ocena roczna
     * 45 - Ocena kadry menadżerskiej
     */
    const DCTPID_NEW_RATING = '47';
    const DCTPID_MANAGER_RATING = '45';
    //id statusu - Roboczy
    const STATE = '104';

    //Opis dokumentu
    const DSCRPT_DOC = 'Ocena pracownika';

    const REGISTER = 'creg_ocena_osoby';

    const PRTPID = 86;


    public function __construct() {
        //
	}
    
    
    /**
     * Sprawdza aktualnych pracowników według struktury organizacyjnej i zwraca w tablicy.
     * 
     * @access public
     * @param PgManager $db
     * @return mixed|boolean|array]
     */
    public static function getAllActualEmployers($db = null){

        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();

        $query = $db->query("SELECT usr_id from orgtree_view where usr_id is not null and orunsm != 'CLIENTS' and ndetpe = 'POST' and is_del = false order by usr_id");

        if(!is_array($query) OR count($query) < 1)
            return false;

        $usr_ids = array();
        foreach($query as $user)
            array_push($usr_ids, $user['usr_id']);
        
        return $usr_ids;
    }

    /**
     * Sprawdza aktualnych pracowników względem rejestru (czy istnieje wpis dla danej osoby).
     * @access public
     * @param array $actualEmployers        
     * @param PgManager $db                 
     */
    
    public static function checkRatingsRegister($actualEmployers, $db = null)
    {
        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();

        require_once('./classes/eDokumentyApi/EDokApi.inc');
        $api = new EDokApi();

        if(!is_array($actualEmployers) OR count($actualEmployers) <1)
            return false;
        
        $data = array();
        foreach($actualEmployers as $usr_id)
        {
            $user = intval($usr_id['usr_id']);

            /**
             *  Rodzaj 1 = Ocena roczna
             *  Rodzaj 2 = Ocena kadry menadżerskiej
             *  Rodzaj 3 = Nie podlega ocena
             */
            $check = $db->query("SELECT id____, prac__, akcept, rok___,d oc_id , rodzaj
            ,CASE WHEN rodzaj = 1 THEN 319 
                  WHEN rodzaj = 2 THEN 296
                  WHEN rodzaj = 3 THEN 311 ELSE 0 END featid
            ,CASE WHEN rodzaj = 1 THEN 47 
                  WHEN rodzaj = 2 THEN 45 
                  WHEN rodzaj = 3 THEN 46 ELSE 0 END dctpid 
            FROM cregisters.creg_ocena_osoby WHERE prac__ IN () AND rok___ = '2018' ");


            $newRegister = GenerateRatings::makeRegister($check,$user, $db, $api);

            array_push($data, $newRegister);
        }
  
        $newRegister = $api->addRegisterEntries('182', $data);
    }

    public static function getSuperior($usr_id, $db = null){

        if(!$db AND !is_a($db, 'PgManager'))
        $db = PgManager::getInstance();

        $orunid = $db->query("SELECT orunid from orgtree_view where usr_id =".$usr_id);
        $superiorOrunid = $db->query("SELECT orunid from orgtree_view where orunid = (select get_exist_superior(".$orunid[0]['orunid']."))"); 


        if(is_array($superiorOrunid))
            return $superiorOrunid[0]['orunid'];
        else
            return false;

    }

    /**
     * Pobiera informacje o ocenach pracowników z konkretnego roku
     * 
     * @param array $users              Tablica z identyfikatorami pracowników
     * @param int|string $year          Rok z którego pobrać dane
     * @param array $columns            Kolumny których dane zostaną pobrane
     * @param PgManager $db             Opcjonalna Instancja PgManagera
     */
    public static function getUsersRating($users, $year, $columns = null, $db = null)
    {
        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();
        
        if(!is_array($users) OR count($users) < 1)
            return false;

        if(!is_array($columns) OR count($columns) < 1)
            $columns = GenerateRatings::DEF_COLUMNS_RAITING; // Domyslne nazwy kolumn

        $year = intval($year);
        if($year == 0)
            return false;

        $users = implode(', ', $users);
        $columns = implode(', ', $columns );

        /**
         *  Rodzaj 1 = Ocena roczna
         *  Rodzaj 2 = Ocena kadry menadżerskiej
         *  Rodzaj 3 = Nie podlega ocena
         * 
         *  Rodzaj oceny wpływa na typ dokumentu. A w konsekwencji są 3 różne cechy, po jednej dla każdego typu dokumentu
         */
        $data = $db->query("SELECT {$columns}
            ,CASE WHEN rodzaj = 1 THEN 319 
                  WHEN rodzaj = 2 THEN 296
                  WHEN rodzaj = 3 THEN 311 ELSE 0 END featid
            ,CASE WHEN rodzaj = 1 THEN 47 
                  WHEN rodzaj = 2 THEN 45 
                  WHEN rodzaj = 3 THEN 46 ELSE 0 END dctpid 
            FROM cregisters.creg_ocena_osoby WHERE prac__ IN ({$users}) AND rok___ = '$year' ");

        return $data;
    }

    /**
     * Funkcja która zwraca, z podanej tablicy użytkowników, tablice z użytkownikami, którzy mają/nie mają (W zależości od flagi) ocene w podanym roku.
     * 
     * @param array $users              Tablica z identyfikatorami pracowników
     * @param int|string $year          Rok z którego ma sprawdzić istnienie ocen
     * @param boolean $return_existing  Flaga booleanowska, która decyduje czy zwrócić użytkowników, którzy mają ocene (True), czy tych który nie mają (False)
     * @param PgManager $db             Opcjonalna Instancja PgManagera
     */
    public static function checkRatingsExistence($users, $year, $return_existing = true, $db = null)
    {
        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();
        
        if(!is_array($users) OR count($users) < 1)
            return false;

        $year = intval($year);
        if($year == 0)
            return false;

        $users_tmp = implode(', ', $users);

        $query = $db->query("SELECT prac__ FROM cregisters.creg_ocena_osoby WHERE prac__ IN ({$users_tmp}) AND rok___ = '$year' ");
        
        $users_with_rating = array();
        foreach($query as $user)
            array_push($users_with_rating, $user['prac__']);

        if($return_existing == true)
            return $users_with_rating;

            
        $users_without_rating = array_diff($users, $users_with_rating);
        
        return $users_without_rating;
    }

    /**
     * Tworzy nowe oceny (Dokument, wpis w rejestrze, cechy) w wskazanym roku na podstawie danych lub z domyslnymi.
     * W przypadku nie podania parametru $data tworzy nowe oceny dla użytkowników w $users.
     * Jeśli podany zostanie parametr $data, to zostanie skopiowana ocena na nowy rok, nie ma znaczenia wtedy parametr $users (Mimo iż jest wymagany @TODO)
     * 
     * @param array $users              Tablica z identyfikatorami pracowników.
     * @param int|string $year          Rok w którym dodać dane.
     * @param array|null $data          Tablica z danymi. Jeśli parametr jest pusty, tworzy nowe oceny - domyslne.
     * @param PgManager $db             Opcjonalna Instancja PgManagera
     */
    public static function addNewRatings($users, $year, $data = null, $db = null)
    {
        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();
    
        if(!is_array($users) OR count($users) < 1)
            return false;

        $year = intval($year);
        if($year == 0)
            return false;

        
        // if(!is_array($data) OR $data)
        //     return false;
        
        $new_ratings = array(); // Tablica z tworzonymi ocenami
        $api = new EDokApi();

        if(!$data)
        {
            // Nowy wpis
            foreach($users as $usr_id)
            {
                if(GenerateRatings::checkManager($usr_id) == true){
                    $featid = GenerateRatings::FEATID_MANAGER_RATING;
                    $dctpid = GenerateRatings::DCTPID_MANAGER_RATING;
                    $rodzaj = GenerateRatings::TYPE_MANAGER_RATING;
                }else{
                    $featid = GenerateRatings::FEATID_NEW_RATING;
                    $dctpid = GenerateRatings::DCTPID_NEW_RATING;
                    $rodzaj = GenerateRatings::TYPE_YEAR_RATING;
                }

                $doc_id = GenerateRatings::createNewDoc($dctpid, $api);
                GenerateRatings::addFeatureToDocument($doc_id, $featid, $usr_id);
                $superiorUser = GenerateRatings::getSuperior($usr_id);
                $new_register = array('prac__' => strval($usr_id), 'akcept' => $superiorUser, 'rodzaj' => $rodzaj, 'rok___' => $year, 'doc_id' => $doc_id );
                GenerateRatings::updateProcedures($doc_id, $superiorUser);
                array_push($new_ratings, $new_register);
            }
        }
        else
        {
            foreach($data as $usr_id){
                if(GenerateRatings::checkManager($usr_id['prac__']) == true){
                    $usr_id['dctpid'] = GenerateRatings::DCTPID_MANAGER_RATING;
                    $usr_id['featid'] = GenerateRatings::FEATID_MANAGER_RATING;
                    $usr_id['rodzaj'] = GenerateRatings::TYPE_MANAGER_RATING;
                }
                // Przepisać. Istnieje już na poprzedni rok.
                $doc_id = GenerateRatings::createNewDoc($usr_id['dctpid'], $api);
                GenerateRatings::addFeatureToDocument($doc_id, $usr_id['featid'] , $usr_id['prac__']);
                $new_register = array('prac__' => $usr_id['prac__'], 'akcept' => $usr_id['akcept'], 'rodzaj' => $usr_id['rodzaj'], 'rok___' => $year, 'doc_id' => $doc_id );
                GenerateRatings::updateProcedures($doc_id, $usr_id['akcept']);
                array_push($new_ratings, $new_register);
            }
        }
        var_dump($new_ratings);
        $register_name = GenerateRatings::REGISTER;
        $reg_id = $db->query("SELECT id____ FROM cregisters.register WHERE tabnam = '{$register_name}'");
        $data = GenerateRatings::addEntryRegisters($reg_id[0]['id____'], $new_ratings);

        return count($data);

    }

    /**
     * 
     */
    public static function createNewDoc($dctpid, $api = null, $data_doc = null){

        if(!$api OR !is_a($api, 'EDokApi'))
            $api = new EDokApi();

        $dctpid = intval($dctpid);
        $state = intval(GenerateRatings::STATE);
        if(is_null($data_doc))
        {
            $data_doc = array(
                    'dscrpt' => GenerateRatings::DSCRPT_DOC,
                    'dctpid' => $dctpid,
                    'tpstid' => $state,
                    'prtpid' => GenerateRatings::PRTPID
            );
        }

        try{
            $doc_id = $api->createDocument($data_doc);
        }catch(Exception $e){
            return false;
        }
        
        return $doc_id;
    }

    /**
     * 
     */
    public static function addFeatureToDocument($doc_id, $featid, $value){

        $doc_id = intval($doc_id);
        if($doc_id == 0)
            return false;
        
        $featid = intval($featid);
        if($featid == 0)
            return false;

        $tblnam = 'types_of_documents_view';    
        $out = FeaturesHelper::setTextValue($featid, $tblnam, $doc_id, $value);
        if($out == 0 OR $out < 0 OR $out == FALSE)
            return false;
        else
            return true;
    }

    /**
     * 
     */
    public static function addEntryRegisters($reg_id, $data, $api = null)
    {        
        if(!$api OR !is_a($api, 'EDokApi'))
            $api = new EDokApi();
        try {
            $ids = $api->addRegisterEntries($reg_id, $data);
        } catch(Exception $fault) {
            return false;
        }
        return $ids;
    }

    public static function updateProcedures($doc_id, $superior, $db = null){
        
        $doc_id = intval($doc_id);
        if($doc_id == 0)
            return false;

        $superior = intval($superior);
        if($superior == 0)
            return false;

        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();

        $procid = $db->query('SELECT procid FROM documents WHERE doc_id ='.$doc_id);
        if(!is_array($procid) OR empty($procid[0]['procid']))
            return false;

        $superior = 'array['.$superior.']';
        $procid = $procid[0]['procid'];
        if(!$abc = $db->query("UPDATE stages SET orgarr = {$superior} WHERE procid = $procid AND is_act = true"))
            var_dump($abc);


    }
    public static function checkYearRegisterEntry($year, $db = null){

        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();

        $year = intval($year);
        if($year == NULL)
            return false;

        $result = $db->query('SELECT (rok___ = \''.$year.'\')::boolean as rok from cregisters.creg_ocena_osoby limit 1');

        var_dump($result,'RESULT');

        if($result[0]['rok'] == "t")
            return true;
        else
            return false;

    }

    public static function checkManager($usr_id,  $db = null){

        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();

        $result = $db->query("SELECT $usr_id IN (SELECT usr_id FROM orgtree_view WHERE ndenam LIKE '%KIEROWNI%' OR ndenam LIKE '%DYREKT%') as x ");
        //select $usr_id in (select usr_id from orgtree_view where ndenam like '%KIEROWNI%' or ndenam like '%DYREKT%')
        var_dump($result,'RESULT1');
    

        if($result[0]['x'] == true)
            return true;
        else
            return false;
    }

    //execute
    public static function generate(){

        $employers = GenerateRatings::getAllActualEmployers();
        
        if(!is_array($employers))
            JScript::alert("Błąd pobrania pracowników ze struktury");

        $db = PgManager::getInstance();

        $current_year = "SELECT to_char(current_date, 'YYYY')::text as rok"; 
        $current_year = $db->query($current_year);

        if(!GenerateRatings::checkYearRegisterEntry($current_year[0]['rok'])){

            $new_employers = GenerateRatings::checkRatingsExistence($employers, $current_year[0]['rok'] - 1, false);
            $emp_with_ratings = GenerateRatings::checkRatingsExistence($employers, $current_year[0]['rok'] - 1, true);
            $data = GenerateRatings::getUsersRating($emp_with_ratings, $current_year[0]['rok'] - 1);
            $correct = GenerateRatings::addNewRatings($new_employers, $current_year[0]['rok'], null);
            $correct2 = GenerateRatings::addNewRatings($emp_with_ratings, $current_year[0]['rok'], $data);
            return true;

        }else{
            JScript::alert("Oceny pracownicze na ".$current_year[0]['rok']."r. już istnieją!");
        }
       
    }
}

if(GenerateRatings::generate()==true)
    JScript::info('Oceny pracownicze zostały wygenerowane poprawnie.', '300px', '150px', 'Oceny pracownicze');      



