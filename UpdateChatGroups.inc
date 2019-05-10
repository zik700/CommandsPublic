<?php
    /*
    *@author Damian Gryń
    * Skrypt do aktualizowania wybranych grup systemowych w chacie względem rejestru, który służy do administracji. 
    * Rejestr zawiera
    * grp_id - służy do powiązania grup systemowych z grupami na chacie.
    * grp_nam - alias, który służy do powiązania rejestru z grupą na chacie.
    * is_sync - flaga czy wskazana grupa ma się synchronizować.
    */
    class UpdateChatGroups{
        /**
         *  Grupy dla których aktualizuje się komunikator  grp_id
         * 1 -pracownicy, 3- Kierownicy, 9 - Handlowcy, 10 - Marketing, 20 - serwisanci, 26 - Monterzy
         */
        // const GROUPS =array(1,3,9,10,20,26);
        
        
        /**
         * Czy usunąć stare kontakty w grupie aktualizującej się
         */
        const DROP_OLD_CONTACTS = true;
        /**
         * tablica z użytkownikami dla grup w GROUPS usr_id
         */
        private $system_users = array();
        /**
         * Nazwy grupy
         */
        private $grp_name = array();
        /**
         * Tablica z grupami, którzy są pobierani z rejestru.
         *
         * @var array
         */
        private $registerEntryGroups = [];
        /**
         * Pobranie identyfikatorów grup i utworzenie stosu.
         *
         */
        public function __construct(){
            $this->db = PgManager::getInstance();
        }
        /**
         * Pobranie z rejestru grup, które mają sie zaktualizować na komunikatorze.
         */
        public function getRegisterGroups(){
            $sync_grp = array();
            $sync_grp = $this->db->query("SELECT grp_id FROM cregisters.creg_chat_groups_sync WHERE is_sync = true;");
            foreach($sync_grp as $s){
                array_push($this->registerEntryGroups, $s['grp_id']);
            }
        }
        /**
         * Odnajduje użytkowników, którzy są we wskazanej grupie systemowej.
         */
        public function findSystemGroupUsers(){
            if(!$this->db AND !is_a($this->db, 'PgManager'))
                $this->db = PgManager::getInstance();
            foreach($this->registerEntryGroups as $grp){
                if (($query = $this->db->query('SELECT u.usr_id FROM users u
                LEFT JOIN users_link_group ul ON (u.usr_id = ul.usr_id)
                WHERE grp_id ='.$grp)) == -1)
                    return false;
                $stack = array();
                foreach($query as $user){
                    array_push($stack, $user['usr_id']);
                }
                // dla indeksu grupy połóż na stosie użytkowników 
                 $this->system_users[$grp] = $stack;
                 var_dump($this->system_users[$grp] = $stack);
            
            } 
            return true;
        }
          
        /**
         * Porównuje identyfikatory grup z rejestru z grp_id w celu powiązania grup systemowych z grupami na komunikatorze.
         */
        public function compareIdGroups()
        {
            if(!$this->db AND !is_a($this->db, 'PgManager'))
                $this->db = PgManager::getInstance();
            foreach($this->registerEntryGroups as $grp){
                if (($query = $this->db->query('SELECT grpnam FROM groups WHERE grp_id ='.$grp)) == -1)
                    return false;
                foreach($query as $name){
                    $this->grp_name[$grp]  = strtoupper($name['grpnam']);
                }
            }
        }
        /**
         * wykonaj update
         */
        public function update(){
            
            if(!$this->db AND !is_a($this->db, 'PgManager'))
                $this->db = PgManager::getInstance();
            // wykonuj dopóki istnieją jabcid-y jeśli koniec to continue
            foreach($this->grp_name as $grp_id => $grp){
             
                if(($query = $this->db->query("SELECT jabcid,owneid FROM jabber_contacts WHERE is_gru = 1 AND upper(con_na) = '$grp'")) == -1)
                    continue;
                foreach($query as $q){
                    $prn_id = $q['jabcid'];
                    $owneid = $q['owneid'];
                    
                    if(self::DROP_OLD_CONTACTS == true)
                        $query = $this->db->query("DELETE FROM jabber_contacts WHERE par_id =".$prn_id);
                        
                    $exists= array();
                    $query = $this->db->query("SELECT jabuid FROM jabber_contacts WHERE par_id =".$prn_id);
                    if($query == -1)
                        $exists = 0;
                    else{
                        foreach($query as $q)
                            array_push($exists, $q['jabuid']);
                    }
                    foreach($this->system_users[$grp_id] as $user){
                        
                        if(($query = $this->db->query("SELECT  * FROM jabber_users WHERE usr_id = $user")) == -1)
                            continue;
                        if($query[0]['jabuid']==$owneid OR ($exists != 0 AND in_array($query[0]['jabuid'],$exists)))
                            continue;
                        
                        $jabuid = $query[0]['jabuid'];
                        $jab_lo = $query[0]['jab_lo'];
                        $sta_id = $query[0]['sta_id'];
        
                        if(($this->db->query("INSERT INTO jabber_contacts(par_id, jabuid, owneid,con_na,is_gru,sta_id) VALUES($prn_id,$jabuid,$owneid,'$jab_lo',0,$sta_id)")) == FALSE)
                            var_dump('co zaszło', $query);
   
                    }
                }
              
            }
        }
}
    $obj = new UpdateChatGroups();
    $obj->getRegisterGroups();
    $obj->findSystemGroupUsers();
    $obj->compareIdGroups();
    $obj->update();
?>