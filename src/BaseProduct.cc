/* 
 * File:   BaseProduct.cc
 * Author: lslezak@suse.cz
 * 
 */

#include "BaseProduct.h"

BaseProduct::BaseProduct(
    const std::string& product_name,
    const zypp::Edition& product_edition,
    const zypp::Arch& product_arch,
    const std::string& source_repo_alias
) :
  name(product_name),
  edition(product_edition),
  arch(product_arch),
  repo_alias(source_repo_alias)
{
}


