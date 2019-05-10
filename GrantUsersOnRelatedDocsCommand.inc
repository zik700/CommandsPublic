<?php
require_once(COMMANDS_DIR.'AbsCommand.inc');
require_once(LIB_PATH.'filesystem/File.inc');
require_once('./classes/eDokumentyApi/EDokApi.inc');

final class GrantUsersOnRelatedDocsCommand extends AbsCommand implements ICommand {



    /**
     * getDescription
     * Metoda zwracająca nazwę komendy (krótki opis). Nazwa ta pojawia się na liście wyboru komend.
     * 
     * @static
     * @access public
     * @return string
     */
    public static function getDescription() {

        return Translator::translate('Uprawnij użytkowników do wszystkich dokumentów powiązanych');

    }



    /**
     * getExpandedDescription
     * Metoda zwracająca dłuższy opis komendy. Może zawierać znaki HTML.
     * 
     * @param string $params - lista parametrów w formacie json (na razie nie jest obsługiwane)
     * @static
     * @access public
     * @return string
     */
    public static function getExpandedDescription($params = NULL) {

        return Translator::translate('');

    }  



    /**
     * getCommandApi
     * Metoda zwracająca API komendy domyślnie jest implementowana przez AbsCommand i zwraca pustą tablicę.
     * Format api to tablica asocjacyjna, której kluczami głównymi są nazwy parametrów (najczęściej 6 znakowe) a każdy z tych kluczy
     * posiada definicję parametru również w postaci tablicy.
     *
     * Definicja parametru określana jest przez 3 atrybuty (klucze)
     *  - (strint)label - krótka nazwa parametru
     *  - (strint)dscrpt - dokładniejszy opis parametru
     *  - (bool)required - oznaczenie czy parametr jest wymagany
     *
     * Dodatkowo w tej tablicy (głównej) może się pojawić klucz example, które podaje przykład listy parametrów - jednak od wersji 
     * 3.7 jest to zbędne gdyż parametry są definiowane w dedykowanym formularzu i przechowywane są w formacie json a nie csv!
     * 
     * @param string $params - lista parametrów w formacie json (na razie nie jest obsługiwane)
     * @static
     * @access public
     * @return array
     */
    public static function getCommandApi($params = NULL) {

    }


    /**
     * Pobiera dokumenty powiązane z bazowym dokumentem.
     *
     * @param int $doc_id
     * @param PgManager $db
     * @return int $documents - id dokumentów.
     */
    public function getRelatedDocuments($doc_id, $db){

        $documents = array();

        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();

        if($doc_id != NULL){
            $query = $db->query("SELECT doc_id FROM doc_link_doc WHERE rel_to = ".$doc_id);

            if($query != -1){
                foreach($query as $q){
                    array_push($documents, $q['doc_id']);
                }
            }
        }

        return $documents;
    }

   /**
    * Pobiera uprawnienia dla grup/użytkowników w zależności od parametru $flag dla dokumentu bazowego i je zwraca.
    *
    * @param int $doc_id
    * @param PgManager $db
    * @param text $flag - 'GROUP' OR 'USER'
    * @return $granted
    */
    public function getAttribsGrantedOnTechDocs($doc_id, $db = null, $flag){

        $granted = array();

        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();

        
         if(!isset($doc_id))
            throw new Exception("doc_id nie istnieje dla getAttribsGrantedOnTechDocs!");

        $doc_id = intval($doc_id);
        if($flag == 'GROUP'){
            $query = $db->query("SELECT grp_id, attrib, rights  FROM doc_link_users WHERE doc_id = {$doc_id} AND grp_id IS NOT NULL ORDER BY grp_id ASC", false, PGSQL_ASSOC);
        }
        elseif($flag == 'USER'){
            $query = $db->query("SELECT usr_id, attrib, rights  FROM doc_link_users WHERE doc_id = {$doc_id} AND usr_id IS NOT NULL ORDER BY usr_id ASC", false, PGSQL_ASSOC);
        }

        if(is_array($query)){
            foreach($query as $q){
                array_push($granted, $q);
            }
        }    

        return $granted;

    }


    /**
     * Przepisz uprawnienia dla użytkowników do powiązanych dokumentów. 
     *
     * @param array $attribUsers - tablica z użytkownikami oraz uprawnieniami.
     * @param array $rel_docs - dokumenty powiązane.
     * @param PgManager $db
     * @return void
     */
    public function grantUsersToRelatedDocs($attribUsers, $rel_docs, $db = null){

        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();
            
        if($attribUsers == NULL AND $rel_docs == NULL)
            throw new Exception("doc_id lub rel_docs nie istnieje dla grantUsersToRelatedDocs!");

        require_once('./classes/eDokumentyApi/EDokApi.inc');  
        $api = new EDokApi();

        foreach($rel_docs as $rel){
            foreach($attribUsers as $usr){
                $user_id = $usr['usr_id'];
                $attrib = $usr['attrib'];
                $rights = $usr['rights'];

                if(empty($user_id) OR  empty($attrib))
                    continue;

                if(is_array(GrantUsersOnRelatedDocsCommand::checkPreviousGrants(null, $user_id, $attrib, $rights, $rel, $db)))
                    continue;

                $sql = "INSERT INTO doc_link_users (usr_id, grp_id, doc_id, attrib, rights, eusrid) values ({$user_id}, null, {$rel}, '{$attrib}', {$rights}, 2);";
                $query = $db->query($sql);
        
            }
        }
    }


    /**
     * Przepisz uprawnienia dla grup do powiązanych dokumentów.
     *
     * @param array $attribGroups
     * @param array $rel_docs
     * @param PgManager $db
     */
    public function grantGroupsToRelatedDocs($attribGroups, $rel_docs, $db = null){

        if(!$db AND !is_a($db, 'PgManager'))
        $db = PgManager::getInstance();
            
        if($attribGroups == NULL AND $rel_docs != NULL)
            throw new Exception("doc_id lub rel_docs nie istnieje dla grantUsersToRelatedDocs!");
        

        foreach($rel_docs as $rel){
            foreach($attribGroups as $grp){
                $group_id = $grp['grp_id'];
                $attrib = $grp['attrib'];
                $rights = $grp['rights'];

                if(empty($grp) OR  empty($attrib))
                    continue;

                if(is_array(GrantUsersOnRelatedDocsCommand::checkPreviousGrants($group_id, null, $attrib, $rights, $rel, $db)))
                    continue;

                $query = $db->query("INSERT INTO doc_link_users(usr_id, grp_id, doc_id, attrib, rights, eusrid) values(null, {$group_id}, {$rel}, '{$attrib}', {$rights}, 2);");
            }
        }        
    }

    /**
     *  Sprawdzam w celu eliminacji redundancji niepotrzebnych wpisów.
     *
     * @param int $group_id
     * @param int $user_id
     * @param text $attrib
     * @param int $rights
     * @param int $doc_id
     * @param PgManager $db
     * @return array $exist
     */
    public function checkPreviousGrants($group_id = null, $user_id = null, $attrib, $rights, $doc_id, $db = null){

        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();


        if(!empty($group_id))
            $exist = $db->query("SELECT doc_id FROM doc_link_users WHERE doc_id = {$doc_id} AND rights = {$rights} AND attrib = '{$attrib}' AND  grp_id = {$group_id}  ");

        if(!empty($user_id))
            $exist = $db->query("SELECT doc_id FROM doc_link_users WHERE doc_id = {$doc_id} AND rights = {$rights} AND attrib = '{$attrib}' AND usr_id = {$user_id} ");

        return $exist;
    }

    /**
     * execute
     * Metoda odpowiedzialna za wykonanie komendy.
     * Poniżej znajdują się też najważniejsze rzeczy jakie trzeba zrobić!
     * 
     * @param Bean $bean - obiekt formularza dokumentu albo sprawy do dyspozycji w tym obiekcie mamy wartości z pól 
     * danego formularza (tabele documents (wraz z dodatkowymi tabelami), processes).
     * Wartości pobieramy metodą $bean->get('dscrpt');
     * @param string $params - lista parametrów jaka została zdefiniowana dla tej komendy w konkretnej procedurze.
     * @access public
     * @return bool|CommandException
     */
    public function execute(Bean $bean, $params) {

        if(!$db AND !is_a($db, 'PgManager'))
            $db = PgManager::getInstance();

        parent::parseParams($params, $bean);        
        // api komendy do weryfikacji parametrów
        $api = self::getCommandApi();
        $doc_id = $bean->getPkeyValue();
        $api = new EDokApi();

        if(!isset($doc_id) OR empty($doc_id))
            throw new Exception("doc_id nie istnieje!");

        $related_docs = GrantUsersOnRelatedDocsCommand::getRelatedDocuments($doc_id, $db);
        $attribUsers = GrantUsersOnRelatedDocsCommand::getAttribsGrantedOnTechDocs($doc_id, $db, 'USER');
        $attribGroups = GrantUsersOnRelatedDocsCommand::getAttribsGrantedOnTechDocs($doc_id, $db, 'GROUP');

        if($related_docs != NULL){
            if($attribUsers != NULL){
                GrantUsersOnRelatedDocsCommand::grantUsersToRelatedDocs($attribUsers, $related_docs, $db);
            }
            if($attribGroups != NULL){
                GrantUsersOnRelatedDocsCommand::grantGroupsToRelatedDocs($attribGroups, $related_docs, $db);
            }
        }
        return true;
    }
}
?>
