<?php
// $Id$

class ShowCompoundImages {

  private $pid = NULL;

  function __construct($pid) {
    //drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
    $this->pid = $pid;
  }
  
  function showViewer()
  {
    
    module_load_include('inc', 'fedora_repository', 'plugins/ShowStreamsInFieldSets');
    module_load_include('inc', 'fedora_repository', 'ObjectHelper');
    
    global $base_url;
    global $user;
    $qs = '';
    if ($user->uid != 0) {
//      $qs = '?uid=' . base64_encode($user->name . ':' . $user->sid);
      $qs = '?uid=' . base64_encode($user->name . ':' . $user->pass);
    }
		$html='';
		if (isset($_SESSION['islandora_solr_query'])) {
			$html .= '<a href="/islandora/solr/search/'.$_SESSION['islandora_solr_query'].'/1/'.$_SESSION['islandora_solr_fq'].($_SESSION['islandora_solr_defType']=='dismax'?'/dismax':'').'?limit='.$_SESSION['islandora_solr_limit'].(isset($_SESSION['islandora_solr_sort'])?'&sort='.$_SESSION['islandora_solr_sort']:'').'&page='.$_SESSION['islandora_solr_page_number'].'">Back to Search Results</a>'; 
			
		} 

    $viewer_url = variable_get('fedora_base_url', 'http://localhost:8080/fedora') . '/get/'. $this->pid . '/ilives:viewerSdef/getViewer'. $qs;
    $html .= '<iframe src="'. $viewer_url . '" scrolling="no" frameborder="0" style="width: 100%; height: 545px;">Errors: unable to load viewer</iframe>';

    if (user_access(ObjectHelper :: $PURGE_FEDORA_OBJECTSANDSTREAMS)) {
      $path = drupal_get_path('module', 'fedora_repository');
      $html .= '<a title="'. t('Edit Metadata') . $this->pid . '" href="'. base_path() . 'fedora/repository/editmetadata/'. 
        $this->pid . '/MODS"><img src="'. $base_url . '/'. $path . '/images/edit.gif" alt="'. t('Edit Metadata') . '" class="icon">'.t('Edit Metadata').'</a>';
    }
   
    return $html;
    
  }
  
  function showParent()
  {
      $query_string = 'select $parent $parentTitle $object $title $content from <#ri>
		       where ($object <dc:title> $title
		       and $object <fedora-model:hasModel> $content
                       and $child <fedora-rels-ext:isPartOf> $parent
                       and $parent <dc:title> $parentTitle
                       and $child <dc:identifier> \''.$this->pid.'\'
		       and $object <fedora-rels-ext:isPartOf> $parent 
		       and $object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>)
		       minus $content <mulgara:is> <info:fedora/fedora-system:FedoraObject-3.0>
		       order by $object';     
		       
     return $this->resourceIndexQuery($query_string);
  }
  
  function showPartOfImages()
  {
   return $this->resourceIndexQuery($query_string);
  }
  
  function resourceIndexQuery($query_string=null,$xslFile='/xsl/ShowCompoundImages.xsl') 
  {
    global $base_url;
    module_load_include('inc', 'fedora_repository', 'CollectionClass');
    
//     if ($query_string == null)
//     {
//       $query_string = 'select $object $title $content from <#ri>
// 		       where ($object <dc:title> $title
// 		       and $object <fedora-model:hasModel> $content
// 		       and ($object <fedora-rels-ext:isPartOf> <info:fedora/' . $this->pid. '> or $object <dc:identifier> \''.$this->pid.'\') 
// 		       and $object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>)
// 		       minus $content <mulgara:is> <info:fedora/fedora-system:FedoraObject-3.0>
// 		       order by $object';    
//     }
// 
//     $collectionClass = new CollectionClass();
//     $content = $collectionClass->getRelatedObjects($this->pid, 100, 0, $query_string);
// 
//     
//      if (isset ($content) && $content != FALSE) 
//      {
//       // php xsl does not seem to work with namespaces so removing it below
//       // I may have just been being stupid here
//        $content = str_ireplace('xmlns="http://www.w3.org/2001/sw/DataAccess/rf1/result"', '', $content);
//      
//        $input = new DomDocument();
//        $input->loadXML($content);	
// 	
//         $results = $input->getElementsByTagName('result');
//         if ($results->length > 0) { 
//          try {
//           $proc = new XsltProcessor();
//           $proc->setParameter('', 'collectionPid', 'collection pid');
//           $proc->setParameter('', 'collectionTitle', 'collection title');
//           $proc->setParameter('', 'baseUrl', $base_url);
//           $proc->setParameter('', 'path', 'path');
//           $proc->setParameter('', 'hitPage', 1);
//           $proc->registerPHPFunctions();
// 	  $xsl = new DOMDocument();
// 	  $xsl->load(drupal_get_path('module', 'fedora_repository').$xslFile);    
// 	  
// 
// 	  
//           $xsl = $proc->importStylesheet($xsl);
//           $newdom = $proc->transformToDoc($input);
//   
//           $objectList = $newdom->saveXML(); //is the xml transformed to html as defined in the xslt associated with the collection object
//           
//           if (!$objectList) {
//             throw new Exception("Invalid XML.");
//          }
//         }
//         catch (Exception $e) {
//          drupal_set_message(t('!e', array('!e' => $e->getMessage())), 'error');
// 
//         }
//       }    
//      }
//     
//     return $objectList;
      return '';
  }
}
